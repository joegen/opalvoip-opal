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


#define new PNEW

#define BAD_TRANSMIT_PKT_MAX 5      // Number of consecutive bad tx packet in below number of seconds
#define BAD_TRANSMIT_TIME_MAX 10    //  maximum of seconds of transmit fails before session is killed

const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;

#define RTP_VIDEO_RX_BUFFER_SIZE 0x100000 // 1Mb
#define RTP_AUDIO_RX_BUFFER_SIZE 0x4000   // 16kb
#define RTP_DATA_TX_BUFFER_SIZE  0x2000   // 8kb
#define RTP_CTRL_BUFFER_SIZE     0x1000   // 4kb


/////////////////////////////////////////////////////////////////////////////

RTP_DataFrame::RTP_DataFrame(PINDEX payloadSz, PINDEX bufferSz)
  : PBYTEArray(max(bufferSz, MinHeaderSize+payloadSz))
  , m_headerSize(MinHeaderSize)
  , m_payloadSize(payloadSz)
  , m_paddingSize(0)
  , m_absoluteTime(0)
  , m_discontinuity(0)
{
  theArray[0] = '\x80'; // Default to version 2
  theArray[1] = '\x7f'; // Default to MaxPayloadType
}


RTP_DataFrame::RTP_DataFrame(const BYTE * data, PINDEX len, bool dynamic)
  : PBYTEArray(data, len, dynamic)
  , m_headerSize(MinHeaderSize)
  , m_payloadSize(0)
  , m_paddingSize(0)
  , m_absoluteTime(0)
  , m_discontinuity(0)
{
  SetPacketSize(len);
}


bool RTP_DataFrame::SetPacketSize(PINDEX sz)
{
  m_discontinuity = 0;

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


void RTP_DataFrame::SetExtension(bool ext)
{
  bool oldState = GetExtension();
  if (ext) {
    theArray[0] |= 0x10;
    if (!oldState) {
      *(DWORD *)(&theArray[m_headerSize]) = 0;
      m_headerSize += 4;
    }
  }
  else {
    theArray[0] &= 0xef;
    if (oldState)
      m_headerSize = MinHeaderSize + 4*GetContribSrcCount();
  }
}


void RTP_DataFrame::SetMarker(bool m)
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


void RTP_DataFrame::CopyHeader(const RTP_DataFrame & other)
{
  if (m_headerSize != other.m_headerSize) {
    if (SetMinSize(other.m_headerSize+m_payloadSize+m_paddingSize) && m_payloadSize > 0)
      memmove(theArray+other.m_headerSize, theArray+m_headerSize, m_payloadSize);
  }

  m_headerSize = other.m_headerSize;
  memcpy(theArray, other.theArray, m_headerSize);
}


BYTE * RTP_DataFrame::GetHeaderExtension(unsigned & id, PINDEX & length, int idx) const
{
  if (!GetExtension())
    return NULL;

  BYTE * ptr = (BYTE *)&theArray[MinHeaderSize + 4*GetContribSrcCount()];
  id = *(PUInt16b *)ptr;
  int extensionSize = *(PUInt16b *)(ptr += 2) * 4;
  ptr += 2;

  if (idx < 0) {
    // RFC 3550 format
    length = extensionSize;
    return ptr + 2;
  }

  if (id == 0xbede) {
    // RFC 5285 one byte format
    while (idx-- > 0) {
      switch (*ptr & 0xf) {
        case 0 :
          ++ptr;
          --extensionSize;
          break;

        case 15 :
          return NULL;

        default :
          int len = (*ptr >> 4)+2;
          ptr += len;
          extensionSize -= len;
      }
      if (extensionSize <= 0)
        return NULL;
    }

    id = *ptr >> 4;
    length = (*ptr & 0xf)+1;
    return ptr+1;
  }

  if ((id&0xfff0) == 0x1000) {
    // RFC 5285 two byte format
    while (idx-- > 0) {
      if (*ptr++ != 0) {
        int len = *ptr + 1;
        ptr += len;
        extensionSize -= len;
      }
    }

    id = *ptr++;
    length = *ptr++;
    return ptr;
  }

  return NULL;
}


bool RTP_DataFrame::SetHeaderExtension(unsigned id, PINDEX length, const BYTE * data, HeaderExtensionType type)
{
  PINDEX headerBase = MinHeaderSize + 4*GetContribSrcCount();
  BYTE * dataPtr = NULL;

  switch (type) {
    case RFC3550 :
      if (PAssert(type < 65536, PInvalidParameter) && PAssert(length < 65535, PInvalidParameter))
        break;
      return false;
    case RFC5285_OneByte :
      if (PAssert(type < 15, PInvalidParameter) && PAssert(length > 0 && length <= 16, PInvalidParameter))
        break;
      return false;
    case RFC5285_TwoByte :
      if (PAssert(type < 256, PInvalidParameter) && PAssert(length <= 256, PInvalidParameter))
        break;
      return false;
  }

  if (type == RFC3550) {
    if (GetExtension())
      SetExtension(false);

    if (!SetExtensionSizeDWORDs((length+3)/4))
      return false;

    BYTE * hdr = (BYTE *)&theArray[headerBase];
    *(PUInt16b *)hdr = (WORD)id;
    dataPtr = hdr += 4;
  }
  else {
    PINDEX oldSize;
    if (GetExtension()) {
      BYTE * hdr = (BYTE *)&theArray[headerBase];
      if (*(PUInt16b *)hdr != (type == RFC5285_OneByte ? 0xbede : 0x1000))
        return false;

      oldSize = GetExtensionSizeDWORDs();
      if (!SetExtensionSizeDWORDs(oldSize + (length + (type == RFC5285_OneByte ? 4 : 5))/4))
        return false;
    }
    else {
      if (!SetHeaderExtension(type == RFC5285_OneByte ? 0xbede : 0x1000,
                              length + (type == RFC5285_OneByte ? 1 : 2),
                              NULL, RFC3550))
        return false;

      oldSize = 0;;
    }

    BYTE * hdr = (BYTE *)&theArray[headerBase+4+oldSize*4];;
    if (type == RFC5285_OneByte) {
      *hdr = (BYTE)((id << 4)|(length-1));
      dataPtr = hdr+1;
    }
    else {
      *hdr++ = (BYTE)id;
      *hdr++ = (BYTE)length;
      dataPtr = hdr;
    }
  }

  if (dataPtr != NULL && data != NULL && length > 0)
    memcpy(dataPtr, data, length);

  SetExtension(true);
  return true;
}


PINDEX RTP_DataFrame::GetExtensionSizeDWORDs() const
{
  if (GetExtension())
    return *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 2];

  return 0;
}


bool RTP_DataFrame::SetExtensionSizeDWORDs(PINDEX sz)
{
  PINDEX oldHeaderSize = m_headerSize;
  m_headerSize = MinHeaderSize + 4*GetContribSrcCount() + (sz+1)*4;
  if (!SetMinSize(m_headerSize+m_payloadSize+m_paddingSize))
    return false;

  memmove(&theArray[m_headerSize], &theArray[oldHeaderSize], m_payloadSize);

  SetExtension(true);
  *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 2] = (WORD)sz;
  return true;
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
  int csrcCount = GetContribSrcCount();

  strm <<  "V="  << GetVersion()
       << " X="  << GetExtension()
       << " M="  << GetMarker()
       << " PT=" << GetPayloadType()
       << " SN=" << GetSequenceNumber()
       << " TS=" << GetTimestamp()
       << " SSRC=" << hex << GetSyncSource() << dec
       << " CSRS-sz=" << csrcCount
       << " hdr-sz=" << GetHeaderSize()
       << " pl-sz=" << GetPayloadSize()
       << '\n';

  for (int csrc = 0; csrc < csrcCount; csrc++)
    strm << "  CSRC[" << csrc << "]=" << GetContribSource(csrc) << '\n';

  if (GetExtension()) {
    for (int idx = -1; ; ++idx) {
      unsigned id;
      PINDEX len;
      BYTE * ptr = GetHeaderExtension(id, len, idx);
      if (ptr == NULL) {
        if (idx < 0)
          continue;
        break;
      }
      strm << "  Header Extension: " << id << " (0x" << hex << id << ")\n"
           << setfill('0') << PBYTEArray(ptr, len, false) << setfill(' ') << dec << '\n';
    }
  }

  strm << "Payload:\n" << hex << setfill('0') << PBYTEArray(GetPayloadPtr(), GetPayloadSize(), false) << setfill(' ') << dec;
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
  SetPayloadSize(fciSize);
  theArray[compoundOffset] &= 0xe0;
  theArray[compoundOffset] |= type;
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

bool RTP_ControlFrame::ReadNextPacket()
{
  // skip over current packet
  compoundOffset += GetPayloadSize() + 4;

  // see if another packet is feasible
  if (compoundOffset + 4 > GetSize())
    return false;

  // check if payload size for new packet is legal
  return compoundOffset + GetPayloadSize() + 4 <= GetSize();
}


bool RTP_ControlFrame::StartNewPacket()
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


unsigned RTP_ControlFrame::FbTMMB::GetBitRate() const
{
  DWORD br = bitRateAndOverhead;
  return ((br >> 9)&0x1ffff)*(1 << (br >> 28));
}


/////////////////////////////////////////////////////////////////////////////

RTPExtensionHeaderInfo::RTPExtensionHeaderInfo()
  : m_id(0)
  , m_direction(Undefined)
{
}


PObject::Comparison RTPExtensionHeaderInfo::Compare(const PObject & obj) const
{
  const RTPExtensionHeaderInfo & other = dynamic_cast<const RTPExtensionHeaderInfo &>(obj);
  if (m_id < other.m_id)
    return LessThan;
  if (m_id > other.m_id)
    return GreaterThan;
  return EqualTo;
}


#if OPAL_SIP
bool RTPExtensionHeaderInfo::ParseSDP(const PString & param)
{
  PINDEX space = param.Find(' ');
  if (space == P_MAX_INDEX)
    return false;

  m_id = param.AsUnsigned();

  PINDEX slash = param.Find('/');
  if (slash > space)
    m_direction = Undefined;
  else {
    PCaselessString dir = param(slash+1, space-1);
    if (dir == "inactive")
      m_direction = Inactive;
    else if (dir == "sendonly")
      m_direction = SendOnly;
    else if (dir == "recvonly")
      m_direction = RecvOnly;
    else if (dir == "sendrecv")
      m_direction = SendRecv;
    else
      return false;
  }

  PINDEX uriPos = space+1;
  while (param[uriPos] == ' ')
    ++uriPos;

  space = param.Find(' ', uriPos);
  if (!m_uri.Parse(param(uriPos, space-1)))
    return false;

  if (space == P_MAX_INDEX)
    m_attributes.MakeEmpty();
  else
    m_attributes = param.Mid(space+1).Trim();

  return true;
}


void RTPExtensionHeaderInfo::OutputSDP(ostream & strm) const
{
  strm << "a=extmap:" << m_id;

  switch (m_direction) {
    case RTPExtensionHeaderInfo::Inactive :
      strm << "/inactive";
      break;
    case RTPExtensionHeaderInfo::SendOnly :
      strm << "/sendonly";
      break;
    case RTPExtensionHeaderInfo::RecvOnly :
      strm << "/recvonly";
      break;
    case RTPExtensionHeaderInfo::SendRecv :
      strm << "/sendrecv";
      break;
    default :
      break;
  }

  strm << ' ' << m_uri;
  if (!m_attributes)
    strm << ' ' << m_attributes;

  strm << "\r\n";
}
#endif // OPAL_SDP

    
/////////////////////////////////////////////////////////////////////////////
