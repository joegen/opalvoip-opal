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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp.h"
#endif

#include <opal_config.h>

#include <rtp/rtp.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

RTP_DataFrame::MetaData::MetaData()
  : m_absoluteTime(0)
  , m_transmitTime(0)
  , m_receivedTime(0)
  , m_discontinuity(0)
{
}


RTP_DataFrame::RTP_DataFrame(PINDEX payloadSz, PINDEX bufferSz)
  : PBYTEArray(max(bufferSz, MinHeaderSize+payloadSz))
  , m_headerSize(MinHeaderSize)
  , m_payloadSize(payloadSz)
  , m_paddingSize(0)
{
  theArray[0] = '\x80'; // Default to version 2
  theArray[1] = '\x7f'; // Default to MaxPayloadType
}


RTP_DataFrame::RTP_DataFrame(const BYTE * data, PINDEX len, bool dynamic)
  : PBYTEArray(data, len, dynamic)
  , m_headerSize(MinHeaderSize)
  , m_payloadSize(0)
  , m_paddingSize(0)
{
  SetPacketSize(len);
}


RTP_DataFrame::RTP_DataFrame(const PBYTEArray data)
  : PBYTEArray(data)
  , m_headerSize(MinHeaderSize)
  , m_payloadSize(0)
  , m_paddingSize(0)
{
  if (SetPacketSize(data.GetSize()))
    m_metaData.m_receivedTime.SetCurrentTime();
  else {
    SetSize(MinHeaderSize);
    theArray[0] = 0; // Make illegal RTP frame
  }
}


bool RTP_DataFrame::SetPacketSize(PINDEX sz)
{
  m_metaData.m_discontinuity = 0;

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
              "smaller than indicated header size, " << sz << " < " << m_headerSize << ": "
           << PHexDump(GetPointer(), std::min(sz, (PINDEX)16)));
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
  m_paddingSize = theArray[sz-1] & 0xff;
  if (m_headerSize + m_paddingSize > sz) {
    PTRACE(2, "RTP\tInvalid RTP packet, padding indicated but not enough data, "
              "size=" << sz << ", pad=" << m_paddingSize << ", header=" << m_headerSize << ": "
           << PHexDump(GetPointer(), std::min(sz, (PINDEX)16)));
    m_paddingSize = 0;
  }

  m_payloadSize = sz - m_headerSize - m_paddingSize;
  return true;
}


PINDEX RTP_DataFrame::GetPacketSize() const
{
  return m_headerSize + m_payloadSize + m_paddingSize;
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


RTP_SyncSourceId RTP_DataFrame::GetContribSource(PINDEX idx) const
{
  PAssert(idx < GetContribSrcCount(), PInvalidParameter);
  return ((PUInt32b *)&theArray[MinHeaderSize])[idx];
}


bool RTP_DataFrame::AdjustHeaderSize(PINDEX newHeaderSize)
{
  if (m_headerSize == newHeaderSize)
    return true;

  PINDEX oldHeaderSize = m_headerSize;
  m_headerSize = newHeaderSize;

  PINDEX packetSize = GetPacketSize(); // New packet size
  if (!SetMinSize(packetSize))
    return false;

  if (packetSize > m_headerSize)
    memmove(theArray+m_headerSize, theArray+oldHeaderSize, packetSize-m_headerSize);
  return true;
}


void RTP_DataFrame::SetContribSource(PINDEX idx, RTP_SyncSourceId src)
{
  PAssert(idx <= 15, PInvalidParameter);

  if (idx >= GetContribSrcCount()) {
    theArray[0] &= 0xf0;
    theArray[0] |= idx+1;
    if (!AdjustHeaderSize(m_headerSize + 4))
      return;
  }

  ((PUInt32b *)&theArray[MinHeaderSize])[idx] = src;
}


void RTP_DataFrame::CopyHeader(const RTP_DataFrame & other)
{
  if (AdjustHeaderSize(other.m_headerSize))
    memcpy(theArray, other.theArray, m_headerSize);
  m_metaData = other.m_metaData;
}


void RTP_DataFrame::Copy(const RTP_DataFrame & other)
{
  PINDEX size = other.GetPacketSize();
  if (SetMinSize(size)) {
    memcpy(theArray, other.theArray, size);
    m_headerSize = other.m_headerSize;
    m_payloadSize = other.m_payloadSize;
    m_paddingSize = other.m_paddingSize;
    m_metaData    = other.m_metaData;
  }
}


void RTP_DataFrame::SetExtension(bool ext)
{
  bool oldState = GetExtension();
  if (ext) {
    if (!oldState && SetExtensionSizeDWORDs(0))
      *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount()] = 0;
  }
  else {
    theArray[0] &= 0xef;
    if (oldState) {
      PINDEX baseHeader = MinHeaderSize + 4*GetContribSrcCount();
      if (m_payloadSize > 0)
        memmove(&theArray[baseHeader], &theArray[m_headerSize], m_payloadSize);
      m_headerSize = baseHeader;
    }
  }
}


PINDEX RTP_DataFrame::GetExtensionSizeDWORDs() const
{
  if (GetExtension())
    return *(PUInt16b *)&theArray[MinHeaderSize + 4*GetContribSrcCount() + 2];

  return 0;
}


bool RTP_DataFrame::SetExtensionSizeDWORDs(PINDEX sz)
{
  PINDEX extHdrOffset = MinHeaderSize + 4*GetContribSrcCount();
  if (!AdjustHeaderSize(extHdrOffset + (sz+1)*4))
    return false;

  theArray[0] |= 0x10;
  *(PUInt16b *)&theArray[extHdrOffset + 2] = (uint16_t)sz;
  return true;
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
      int len;
      switch (*ptr >> 4) {
        case 0 :
          len = 1;
          break;
        case 15 :
          return NULL;
        default :
          len = (*ptr & 0xf)+2;
      }
      if (extensionSize <= len)
        return NULL;
      extensionSize -= len;
      ptr += len;
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
        if (extensionSize <= len)
          return NULL;
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


BYTE * RTP_DataFrame::GetHeaderExtension(HeaderExtensionType type, unsigned idToFind, PINDEX & length) const
{
  if (idToFind > MaxHeaderExtensionId)
    return NULL;

  if (!GetExtension())
    return NULL;

  BYTE * ptr = (BYTE *)&theArray[MinHeaderSize + 4*GetContribSrcCount()];
  unsigned idPresent = *(PUInt16b *)ptr;
  PINDEX extensionSize = *(PUInt16b *)(ptr += 2) * 4;
  ptr += 2;

  switch (type) {
    case RFC3550 :
      if (idPresent != idToFind)
        return NULL;
      length = extensionSize;
      return ptr + 2;

    case RFC5285_OneByte :
      if (idPresent != 0xbede)
        return NULL;

      for (;;) {
        idPresent = *ptr >> 4;
        length = (*ptr & 0xf)+1;
        if (idPresent == idToFind)
          return ptr+1;

        switch (idPresent) {
          case 0 :
            break;
          case 15 :
            return NULL;
          default :
            ++length;
        }
        if (extensionSize <= length)
          return NULL;

        extensionSize -= length;
        ptr += length;
      }

    case RFC5285_TwoByte :
      if ((idPresent&0xfff0) != 0x1000)
        return NULL;

      for (;;) {
        idPresent = *ptr++;
        --extensionSize;

        if (idPresent == 0) {
          if (extensionSize > 0)
            continue;
          return NULL;
        }

        length = *ptr++;
        --extensionSize;

        if (idPresent == idToFind)
          return ptr;

        if (extensionSize <= length)
          return NULL;
        extensionSize -= length;
        ptr += length;
      }

    default :
      break;
  }

  return NULL;
}


bool RTP_DataFrame::SetHeaderExtension(unsigned id, PINDEX length, const BYTE * data, HeaderExtensionType type)
{
  if (!PAssert(data != NULL || length == 0, PInvalidParameter))
    return false;

  PINDEX baseHeaderSize = MinHeaderSize + 4*GetContribSrcCount();
  BYTE * currentExtension = NULL;

  unsigned oldId;
  PINDEX extensionSize;
  if (GetExtension()) {
    oldId = GetAs<PUInt16b>(baseHeaderSize);
    extensionSize = GetAs<PUInt16b>(baseHeaderSize+2);
  }
  else {
    oldId = UINT_MAX; // definitely won't match anything
    extensionSize = 0;
  }

  switch (type) {
    case RFC3550 :
      // Have maximum length of 16 bit field header, and it is in DWORDs, that's a 265kbyte header extension, really?
      if (!PAssert(id <= MaxHeaderExtensionId && length < (1 << 16) * 4, PInvalidParameter))
        return false;

      // Set the header size and allocate extra space as needed
      if (!SetExtensionSizeDWORDs((length + 3) / 4))
        return false;

      // Primitive, and there can be only one.
      *(PUInt16b *)(theArray + baseHeaderSize) = (uint16_t)id;
      memcpy(theArray + baseHeaderSize + 4, data, length);
      return true;

    case RFC5285_OneByte :
      if (!PAssert(id > 0 && id <= MaxHeaderExtensionIdOneByte && length > 0 && length <= 16, PInvalidParameter))
        return false;

      if (oldId != 0xbede) {
        /* There was not a RFC3550 header extension at all, or was not of the
           correct type for RFC5285 One Byte mode, maybe two byte mode for
           example. So overwrite it, setting new RFC3550 header extension size
           and copying in our one new extension. */
        if (!SetExtensionSizeDWORDs((length + 1 + 3) / 4))
          return false;

        *(PUInt16b *)(theArray + baseHeaderSize) = 0xbede;
        theArray[baseHeaderSize + 4] = (BYTE)((id << 4)|(length-1));
        memcpy(theArray + baseHeaderSize + 5, data, length);
        return true;
      }

      // Search for the end of the existing headers, checking if id already there
      currentExtension = (BYTE *)theArray + baseHeaderSize + 4;
      for (;;) {
        unsigned currentId = *currentExtension >> 4;
        PINDEX currentLen = (*currentExtension & 0xf)+1;
        if (currentId == id) {
          // Already present, so overwrite it, but we don't support a change in size
          if (!PAssert(length == currentLen, PSTRSTRM("Header Extension size changed: old=" << currentLen << " new=" << length)))
            return false;
          memcpy(currentExtension+1, data, length);
          return true;
        }

        // By definition, the end of extensions, leave currentExtension pointing at it and overwrite
        if (currentId == 15)
          break;

        if (currentId > 0)
          ++currentLen; // Not pad, so length is really 1 to 16

        currentExtension += currentLen; // Add in before the break, so is pointing at end of all extensions
        if (extensionSize <= currentLen)
          break;
        extensionSize -= currentLen;
      }
      break;

    case RFC5285_TwoByte :
      if (PAssert(id > 0 && id <= MaxHeaderExtensionIdTwoByte && length < 256, PInvalidParameter))
        return false;

      if ((oldId & 0xfff0) != 0x1000) {
        /* There was not a RFC3550 header extension at all, or was not of the
           correct type for RFC5285 Two Byte mode, maybe one byte mode for
           example. So overwrite it, setting new RFC3550 header extension size
           and copying in our one new extension. */
        if (!SetExtensionSizeDWORDs((length + 2 + 3) / 4))
          return false;

        *(PUInt16b *)(theArray + baseHeaderSize) = 0x1000;
        theArray[baseHeaderSize + 4] = (BYTE)id;
        theArray[baseHeaderSize + 5] = (BYTE)length;
        memcpy(theArray + baseHeaderSize + 6, data, length);
        return true;
      }

      // Search for the end of the existing headers, checking if id already there
      currentExtension = (BYTE *)theArray + baseHeaderSize + 4;
      for (;;) {
        unsigned currentId = *currentExtension++;
        --extensionSize;

        if (currentId == 0) {
          if (extensionSize > 0)
            continue;
          break;
        }

        PINDEX currentLen = *currentExtension++;
        --extensionSize;

        if (currentId == id) {
          // Already present, so overwrite it, but we don't support a change in size
          if (!PAssert(length == currentLen, PSTRSTRM("Header Extension size changed: old=" << currentLen << " new=" << length)))
            return false;
          memcpy(currentExtension, data, length);
          return true;
        }

        currentExtension += currentLen; // Add in before the break, so is pointing at end of all extensions
        if (extensionSize <= currentLen)
          break;
        extensionSize -= currentLen;
      }
  }

  // Calculate new RFC3550 header extension size, as we append new one to the end
  PINDEX previousHeadersSize = PAssertNULL(currentExtension) - (BYTE *)&theArray[baseHeaderSize + 2];
  PINDEX newHeadersSize = previousHeadersSize + (type == RFC5285_OneByte ? 1 : 2) + length; // New appended header size
  if (!SetExtensionSizeDWORDs((newHeadersSize + 3)/4)) // Converted to whole DWORDs
    return false;

  // Set the header extensions header
  if (type == RFC5285_OneByte)
    *currentExtension++ = (BYTE)((id << 4)|(length-1));
  else {
    *currentExtension++ = (BYTE)id;
    *currentExtension++ = (BYTE)length;
  }

  memcpy(currentExtension, data, length);
  return true;
}


bool RTP_DataFrame::SetPayloadSize(PINDEX sz)
{
  m_payloadSize = sz;
  return SetMinSize(GetPacketSize());
}


bool RTP_DataFrame::SetPayload(const BYTE * data, PINDEX sz)
{
  if (!SetPayloadSize(sz))
    return false;

  memcpy(GetPayloadPtr(), data, sz);
  return true;
}


bool RTP_DataFrame::SetPaddingSize(PINDEX paddingSize)
{
  if (paddingSize == 0) {
    SetPadding(false);
    m_paddingSize = 0;
    return true;
  }

  if (!PAssert(paddingSize < 255, PInvalidParameter))
    return false;

  SetPadding(true);
  m_paddingSize = paddingSize+1;

  PINDEX packetSize = GetPacketSize();
  if (!SetMinSize(packetSize))
    return false;

  theArray[packetSize-1] = (BYTE)m_paddingSize;
  return true;
}


#if PTRACING
void RTP_DataFrame::PrintOn(ostream & strm) const
{
  bool summary = strm.width() == 1 || strm.width() == -1;

  int csrcCount = GetContribSrcCount();

  strm <<  "V="  << GetVersion()
       << " SSRC=" << RTP_TRACE_SRC(GetSyncSource())
       << " M="  << GetMarker()
       << " PT=" << GetPayloadType()
       << " SN=" << GetSequenceNumber()
       << " TS=" << GetTimestamp()
       << " absT=" << GetAbsoluteTime().AsString(PTime::TodayFormat);
  if (csrcCount > 0)
    strm  << " CSRS=" << csrcCount;
  strm << " hdr-sz=" << m_headerSize
       << " pay-sz=" << m_payloadSize;
  if (m_paddingSize > 0)
    strm << " pad-sz=" << m_paddingSize;

  if (summary) {
    if (GetExtension()) {
      strm << " hdr-ext=";

      unsigned id;
      PINDEX len;
      int count = 0;
      while (GetHeaderExtension(id, len, count) != NULL)
        ++count;
      if (count > 0)
        strm << count;
      else if (GetHeaderExtension(id, len, -1) != NULL)
        strm << "0x" << hex << id << dec;
      else
        strm << "None";
    }
    return;
  }

  strm << '\n';

  for (int csrc = 0; csrc < csrcCount; csrc++)
    strm << "  CSRC[" << csrc << "]=" << RTP_TRACE_SRC(GetContribSource(csrc)) << '\n';

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
      strm << "  Header Extension: " << id << " (0x" << hex << id << ")\n" << PHexDump(ptr, len) << '\n';
    }
  }

  strm << "Payload:\n" << PHexDump(GetPayloadPtr(), GetPayloadSize(), false);
}


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


void RTP_ReceiverReport::PrintOn(ostream & strm) const
{
  strm << "SSRC=" << RTP_TRACE_SRC(sourceIdentifier)
       << " fraction=" << fractionLost
       << " lost=" << totalLost
       << " last_seq=" << lastSequenceNumber
       << " jitter=" << jitter
       << " lsr=" << lastTimestamp.AsString(PTime::TodayFormat)
       << " dlsr=" << delay;
}


void RTP_SenderReport::PrintOn(ostream & strm) const
{
  strm << "SSRC=" << RTP_TRACE_SRC(sourceIdentifier)
       << " ntp=" << realTimestamp.AsString(PTime::TodayFormat)
       << " (" << (realTimestamp - PTime()) << ")"
          " rtp=" << rtpTimestamp
       << " psent=" << packetsSent
       << " osent=" << octetsSent;
}


void RTP_DelayLastReceiverReport::PrintOn(ostream & strm) const
{
  strm << "DLRR: lrr=" << m_lastTimestamp.AsString(PTime::LoggingFormat) << ", dlrr=" << m_delay;
}


void RTP_SourceDescription::PrintOn(ostream & strm) const
{
  static const char * const DescriptionNames[RTP_ControlFrame::NumDescriptionTypes] = {
    "END", "CNAME", "NAME", "EMAIL", "PHONE", "LOC", "TOOL", "NOTE", "PRIV"
  };

  strm << "\n  SSRC=" << RTP_TRACE_SRC(sourceIdentifier);
  for (POrdinalToString::const_iterator it = items.begin(); it != items.end(); ++it) {
    unsigned typeNum = it->first;
    strm << "\n    item[" << typeNum << "]: type=";
    if (typeNum < PARRAYSIZE(DescriptionNames))
      strm << DescriptionNames[typeNum];
    else
      strm << typeNum;
    strm << " data=\"" << it->second << '"';
  }
}
#endif // PTRACING


/////////////////////////////////////////////////////////////////////////////

RTP_ControlFrame::RTP_ControlFrame(PINDEX sz)
  : PBYTEArray(sz)
  , m_packetSize(0)
  , m_compoundOffset(0)
  , m_payloadSize(0)
{
}


RTP_ControlFrame::RTP_ControlFrame(const BYTE * data, PINDEX size, bool dynamic)
  : PBYTEArray(data, size, dynamic)
  , m_packetSize(size)
  , m_compoundOffset(0)
  , m_payloadSize(0)
{
}


#if PTRACING
void RTP_ControlFrame::PrintOn(ostream & strm) const
{
  strm << "RTCP frame, " << m_packetSize << " bytes:\n"
       << setprecision(2) << PHexDump(*this, false);
}
#endif


bool RTP_ControlFrame::IsValid() const
{
  if (m_packetSize < m_compoundOffset + 4)
    return false;

  if (m_packetSize < m_compoundOffset + GetPayloadSize() + 4)
    return false;

  PayloadTypes type = GetPayloadType();
  return type >= e_FirstValidPayloadType && type <= e_LastValidPayloadType;
}


bool RTP_ControlFrame::SetPacketSize(PINDEX size)
{
  m_compoundOffset = 0;
  m_payloadSize = 0;

  m_packetSize = size;
  SetMinSize(size);

  return IsValid();
}


void RTP_ControlFrame::SetCount(unsigned count)
{
  PAssert(count < 32, PInvalidParameter);
  theArray[m_compoundOffset] &= 0xe0;
  theArray[m_compoundOffset] |= count;
}


RTP_ControlFrame::FbHeader * RTP_ControlFrame::AddFeedback(PayloadTypes pt, unsigned type, PINDEX fciSize)
{
  PAssert(type < 32, PInvalidParameter);

  StartNewPacket(pt);
  SetPayloadSize(fciSize);
  theArray[m_compoundOffset] &= 0xe0;
  theArray[m_compoundOffset] |= type;
  return (FbHeader *)(theArray + m_compoundOffset + 4); 
}


void RTP_ControlFrame::SetPayloadType(PayloadTypes pt)
{
  PAssert(pt >= e_FirstValidPayloadType && pt <= e_LastValidPayloadType, PInvalidParameter);
  theArray[m_compoundOffset+1] = (BYTE)pt;
}


bool RTP_ControlFrame::SetPayloadSize(PINDEX sz)
{
  m_payloadSize = sz;

  // compound size is in words, rounded up to nearest word
  unsigned compoundDWORDs = (m_payloadSize + 3)/4;
  PAssert(compoundDWORDs <= 0x3fff, PInvalidParameter);

  // transmitted length is the offset of the last compound block
  // plus the compound length of the last block
  m_packetSize = m_compoundOffset + (compoundDWORDs+1)*4;

  // make sure buffer is big enough for previous packets plus packet header plus payload
  if (!SetMinSize(m_packetSize))
    return false;

  // put the new compound size into the packet (always at offset 2)
  *(PUInt16b *)&theArray[m_compoundOffset+2] = (uint16_t)compoundDWORDs;
  return true;
}


BYTE * RTP_ControlFrame::GetPayloadPtr() const 
{ 
  // payload for current packet is always one DWORD after the current compound start
  if ((GetPayloadSize() == 0) || ((m_compoundOffset + 4) >= m_packetSize))
    return NULL;
  return (BYTE *)(theArray + m_compoundOffset + 4); 
}


bool RTP_ControlFrame::ReadNextPacket()
{
  // skip over current packet
  m_compoundOffset += GetPayloadSize() + 4;

  // see if another packet is feasible
  if (m_compoundOffset + 4 > m_packetSize)
    return false;

  // check if payload size for new packet is legal
  return m_compoundOffset + GetPayloadSize() + 4 <= m_packetSize;
}


bool RTP_ControlFrame::StartNewPacket(PayloadTypes pt)
{
  m_compoundOffset = m_packetSize;

  // allocate storage for new packet header
  if (!SetMinSize(m_compoundOffset + 4))
    return false;

  theArray[m_compoundOffset] = '\x80';      // Set version 2
  theArray[m_compoundOffset+1] = (BYTE)pt;  // Payload type
  return SetPayloadSize(0);                 // payload is zero bytes
}

void RTP_ControlFrame::EndPacket()
{
  // all packets must align to DWORD boundaries
  while ((m_payloadSize & 3) != 0) {
    theArray[m_compoundOffset + 4 + m_payloadSize] = 0;
    ++m_payloadSize;
  }

  m_packetSize = m_compoundOffset + 4 + m_payloadSize;
}


bool RTP_ControlFrame::ParseGoodbye(RTP_SyncSourceId & ssrc, RTP_SyncSourceArray & csrc, PString & msg)
{
  size_t size = GetPayloadSize();
  size_t count = GetCount();
  size_t msgOffset = count*sizeof(PUInt32b);
  if (count == 0 || size < msgOffset)
    return false;

  const BYTE * payload = GetPayloadPtr();
  ssrc = *(const PUInt32b *)payload;

  csrc.resize(count);
  for (size_t i = 0; i < count; i++)
    csrc[i] = ((const PUInt32b *)payload)[i+1];

  if (size == msgOffset)
    return true;

  size_t msgLen = payload[msgOffset];
  if (size < msgOffset + msgLen + 1)
    return false;

  msg = PString((const char *)(payload+msgOffset+1), msgLen);
  return true;
}


RTP_SenderReport::RTP_SenderReport()
  : sourceIdentifier(0)
  , ntpPassThrough(0)
  , realTimestamp(0)
  , rtpTimestamp(0)
  , packetsSent(0)
  , octetsSent(0)
{
}


RTP_SenderReport::RTP_SenderReport(const RTP_ControlFrame::SenderReport & sr)
  : sourceIdentifier(sr.ssrc)
  , ntpPassThrough(sr.ntp_ts)
  , realTimestamp(0)
  , rtpTimestamp(sr.rtp_ts)
  , packetsSent(sr.psent)
  , octetsSent(sr.osent)
{
  realTimestamp.SetNTP(ntpPassThrough);
}


RTP_ReceiverReport::RTP_ReceiverReport(const RTP_ControlFrame::ReceiverReport & rr, uint64_t ntpPassThru)
  : sourceIdentifier(rr.ssrc)
  ,  fractionLost(rr.fraction)
  , totalLost(rr.GetLostPackets())
  , lastSequenceNumber(rr.last_seq)
  , jitter(rr.jitter)
  , lastTimestamp(0)
  , delay(((uint32_t)rr.dlsr*1000LL)/65536) // units of 1/65536 seconds
{
  if ((uint32_t)(ntpPassThru>>16) == rr.lsr)
    lastTimestamp.SetNTP(ntpPassThru);
}


RTP_DelayLastReceiverReport::RTP_DelayLastReceiverReport(const RTP_ControlFrame::DelayLastReceiverReport::Receiver & dlrr)
  : m_ssrc(dlrr.ssrc)
  , m_lastTimestamp(0)
  , m_delay(((uint32_t)dlrr.dlrr*1000LL)/65536) // units of 1/65536 seconds
{
  m_lastTimestamp.SetNTP((uint64_t)(uint32_t)dlrr.lrr << 16);
}


/////////////////////////////////////////////////////////////////////////////

bool RTP_ControlFrame::ParseReceiverReport(RTP_SyncSourceId & ssrc, const ReceiverReport * & rr, unsigned & count)
{
  size_t size = GetPayloadSize();
  count = GetCount();
  if (size < sizeof(PUInt32b)+count*sizeof(ReceiverReport))
    return false;

  const PUInt32b * sender = (const PUInt32b *)GetPayloadPtr();
  ssrc = *sender;
  rr = (const RTP_ControlFrame::ReceiverReport *)(sender+1);
  return true;
}


RTP_ControlFrame::ReceiverReport * RTP_ControlFrame::AddReceiverReport(RTP_SyncSourceId ssrc, unsigned receivers)
{
  StartNewPacket(e_ReceiverReport);
  SetPayloadSize(sizeof(PUInt32b) + receivers*sizeof(ReceiverReport));  // length is SSRC of packet sender plus RRs
  SetCount(receivers);
  PUInt32b * sender = (PUInt32b *)GetPayloadPtr();

  // add the SSRC to the start of the payload
  *sender = ssrc;

  // add the RR's after the SSRC
  return receivers > 0 ? (ReceiverReport *)(sender + 1) : NULL;
}


bool RTP_ControlFrame::ParseSenderReport(RTP_SenderReport & txReport, const ReceiverReport * & rr, unsigned & count)
{
  size_t size = GetPayloadSize();
  count = GetCount();
  if (size < sizeof(SenderReport) + count*sizeof(ReceiverReport))
    return false;

  const SenderReport * sr = (const SenderReport *)GetPayloadPtr();
  txReport = RTP_SenderReport(*sr);

  rr = (const RTP_ControlFrame::ReceiverReport *)(sr + 1);
  return true;
}


void RTP_ControlFrame::StartSourceDescription(RTP_SyncSourceId src)
{
  // extend payload to include SSRC + END
  SetPayloadSize(m_payloadSize + 4 + 1);  
  SetPayloadType(e_SourceDescription);
  SetCount(GetCount()+1); // will be incremented automatically

  // get ptr to new item SDES
  BYTE * payload = GetPayloadPtr();
  *(PUInt32b *)payload = src;
  payload[4] = e_END;
}


RTP_ControlFrame::ReceiverReport * RTP_ControlFrame::AddSenderReport(RTP_SyncSourceId ssrc,
                                                                     const PTime & ntp,
                                                                     RTP_Timestamp ts,
                                                                     unsigned packets,
                                                                     uint64_t octets,
                                                                     unsigned receivers)
{
  // send SR and RR
  StartNewPacket(e_SenderReport);
  SetPayloadSize(sizeof(SenderReport) + receivers*sizeof(ReceiverReport));  // length is SSRC of packet sender plus SR + RRs
  SetCount(receivers);

  // add the SR
  SenderReport * sr = (SenderReport *)GetPayloadPtr();
  sr->ssrc = ssrc;
  sr->ntp_ts = ntp.GetNTP();
  sr->rtp_ts = ts;
  sr->psent  = packets;
  sr->osent  = (uint32_t)octets;

  // add the RR's after the SR
  return receivers > 0 ? (ReceiverReport *)(sr + 1) : NULL;
}


void RTP_ControlFrame::AddReceiverReferenceTimeReport(RTP_SyncSourceId ssrc, const PTime & ntp)
{
  // Did not send SR, so send RRTR extended report
  StartNewPacket(RTP_ControlFrame::e_ExtendedReport);
  SetPayloadSize(sizeof(PUInt32b) + sizeof(ReceiverReferenceTimeReport));
  PUInt32b * sender = (PUInt32b *)GetPayloadPtr();

  // add the SSRC to the start of the payload
  *sender = ssrc;

  // add the RRTR after the SSRC
  ReceiverReferenceTimeReport * rrtr = (ReceiverReferenceTimeReport *)(sender+1);
  rrtr->bt = 4;
  rrtr->length = 2;
  rrtr->ntp = ntp.GetNTP();
  EndPacket();
}


RTP_ControlFrame::DelayLastReceiverReport::Receiver *
          RTP_ControlFrame::AddDelayLastReceiverReport(RTP_SyncSourceId ssrc, unsigned receivers)
{
  // Did not send RR, so send DLRR extended report
  StartNewPacket(RTP_ControlFrame::e_ExtendedReport);
  PINDEX xrSize = sizeof(RTP_ControlFrame::DelayLastReceiverReport) + (receivers-1)*sizeof(RTP_ControlFrame::DelayLastReceiverReport::Receiver);
  SetPayloadSize(sizeof(PUInt32b) + xrSize);
  PUInt32b * sender = (PUInt32b *)GetPayloadPtr();

  // add the SSRC to the start of the payload
  *sender = ssrc;

  // add the RRTR after the SSRC
  DelayLastReceiverReport * dlrr = (DelayLastReceiverReport *)(sender+1);
  dlrr->bt = 5;
  dlrr->length = (WORD)(xrSize/4-1);

  return dlrr->m_receiver;
}


void RTP_ControlFrame::AddDelayLastReceiverReport(DelayLastReceiverReport::Receiver & dlrr,
                                                 RTP_SyncSourceId ssrc,
                                                 const PTime & ntp,
                                                 const PTimeInterval & delay)
{
  dlrr.ssrc = ssrc;
  dlrr.lrr = (uint32_t)(ntp.GetNTP()>>16);
  dlrr.dlrr = (uint32_t)(delay.GetMilliSeconds()*65536/1000);
}


void RTP_ControlFrame::AddSourceDescriptionItem(unsigned type, const PString & data)
{
  // get ptr to new item, remembering that END was inserted previously
  BYTE * payload = GetPayloadPtr() + m_payloadSize - 1;

  // length of new item
  PINDEX dataLength = data.GetLength();

  // add storage for new item (note that END has already been included)
  SetPayloadSize(m_payloadSize + 1 + 1 + dataLength);

  // insert new item
  payload[0] = (BYTE)type;
  payload[1] = (BYTE)dataLength;
  memcpy(payload+2, (const char *)data, dataLength);

  // insert new END
  payload[2+dataLength] = (BYTE)e_END;
}


bool RTP_ControlFrame::ParseSourceDescriptions(RTP_SourceDescriptionArray & descriptions)
{
  size_t size = GetPayloadSize();
  size_t count = GetCount();
  if (size < count*sizeof(SourceDescription))
    return false;

  size_t uiSizeCurrent = 0;   /* current size of the items already parsed */

  const SourceDescription * sdes = (const SourceDescription *)GetPayloadPtr();
  for (size_t srcIdx = 0; srcIdx < count; srcIdx++) {
    if (uiSizeCurrent >= size)
      return false;

    descriptions.SetAt(srcIdx, new RTP_SourceDescription(sdes->src));
    const SourceDescription::Item * item = sdes->item;
    while (item->type != e_END) {
      descriptions[srcIdx].items.SetAt(item->type, PString(item->data, item->length));
      uiSizeCurrent += item->GetLengthTotal();
      if (uiSizeCurrent >= size)
        break;

      item = item->GetNextItem();
    }
  }

  return true;
}


void RTP_ControlFrame::AddSourceDescription(RTP_SyncSourceId ssrc,
                                            const PString & cname,
                                            const PString & toolName,
                                            bool endPacket)
{
  StartNewPacket(RTP_ControlFrame::e_SourceDescription);

  SetCount(0); // will be incremented automatically
  StartSourceDescription(ssrc);
  AddSourceDescriptionItem(RTP_ControlFrame::e_CNAME, cname);
  AddSourceDescriptionItem(RTP_ControlFrame::e_TOOL, toolName);

  if (endPacket)
    EndPacket();
}


void RTP_ControlFrame::AddIFR(RTP_SyncSourceId syncSourceIn)
{
  StartNewPacket(e_IntraFrameRequest);
  SetPayloadSize(4);

  // Insert SSRC
  SetCount(1);
  BYTE * payload = GetPayloadPtr();
  *(PUInt32b *)payload = syncSourceIn;
}


ostream & operator<<(ostream & strm, const RTP_ControlFrame::LostPacketMask & mask)
{
  for (RTP_ControlFrame::LostPacketMask::const_iterator it = mask.begin(); it != mask.end(); ++it) {
    if (it != mask.begin())
      strm << ',';
    strm << *it;
  }
  return strm;
}


void RTP_ControlFrame::AddNACK(RTP_SyncSourceId syncSourceOut, RTP_SyncSourceId syncSourceIn, const LostPacketMask & lostPackets)
{
  if (lostPackets.empty())
    return;

  FbNACK * nack;
  AddFeedback(e_TransportLayerFeedBack, e_TransportNACK, nack);

  nack->senderSSRC = syncSourceOut;
  nack->mediaSSRC = syncSourceIn;

  LostPacketMask::const_iterator it = lostPackets.begin();
  size_t i = 0;
  nack->fld[i].packetID = (uint16_t)*it++;
  unsigned bitmask = 0;
  while (it != lostPackets.end()) {
    unsigned bit = (*it - nack->fld[i].packetID)-1;
    if (bit < 16)
      bitmask |= (1 << bit);
    else {
      nack->fld[i].bitmask = (uint16_t)bitmask;
      bitmask = 0;

      ++i;
      SetPayloadSize(sizeof(FbNACK)+sizeof(FbNACK::Field)*i);
      nack->fld[i].packetID = (uint16_t)*it;
    }
    ++it;
  }
  nack->fld[i].bitmask = (uint16_t)bitmask;
}


bool RTP_ControlFrame::ParseNACK(RTP_SyncSourceId & senderSSRC, RTP_SyncSourceId & targetSSRC, LostPacketMask & lostPackets)
{
  size_t size = GetPayloadSize();
  if (size < sizeof(FbNACK))
    return false;

  const FbNACK * nack = (const FbNACK *)GetPayloadPtr();
  senderSSRC = nack->senderSSRC;
  targetSSRC = nack->mediaSSRC;

  size_t count = (size - sizeof(FbNACK)) / sizeof(FbNACK::Field) + 1;

  lostPackets.clear();
  for (size_t i = 0; i < count; ++i) {
    RTP_SequenceNumber pktId = nack->fld[i].packetID;
    lostPackets.insert(pktId);
    for (RTP_SequenceNumber bit = 0; bit < 16; ++bit) {
      if (nack->fld[i].bitmask & (1 << bit))
        lostPackets.insert(pktId + bit + 1);
    }
  }

  return true;
}


static void SplitBitRate(unsigned bitRate, unsigned & exponent, unsigned & mantissa, unsigned mantissaBits)
{
  exponent = 0;
  mantissa = bitRate;
  while (mantissa >= mantissaBits) {
    mantissa >>= 1;
    ++exponent;
  }
}


void RTP_ControlFrame::AddTMMB(RTP_SyncSourceId syncSourceOut, RTP_SyncSourceId syncSourceIn, unsigned maxBitRate, unsigned overhead, bool notify)
{
  FbTMMB * tmmb;
  AddFeedback(e_TransportLayerFeedBack, notify ? e_TMMBN : e_TMMBR, tmmb);

  tmmb->senderSSRC = syncSourceOut;
  tmmb->mediaSSRC = 0;
  tmmb->requestSSRC = syncSourceIn;

  unsigned exponent, mantissa;
  SplitBitRate(maxBitRate, exponent, mantissa, 1<<17);
  tmmb->bitRateAndOverhead = overhead | (mantissa << 9) | (exponent << 26);
}


bool RTP_ControlFrame::ParseTMMB(RTP_SyncSourceId & senderSSRC, RTP_SyncSourceId & targetSSRC, unsigned & maxBitRate, unsigned & overhead)
{
  size_t size = GetPayloadSize();
  if (size < sizeof(FbTMMB))
    return false;

  const FbTMMB * tmmb = (const FbTMMB *)GetPayloadPtr();
  senderSSRC = tmmb->senderSSRC;
  targetSSRC = tmmb->requestSSRC;
  maxBitRate = ((tmmb->bitRateAndOverhead >> 9)&0x1ffff)*(1 << (tmmb->bitRateAndOverhead >> 26));
  overhead = tmmb->bitRateAndOverhead & 0x1ff;
  return true;
}


RTP_TransportWideCongestionControl::RTP_TransportWideCongestionControl()
  : m_rtcpSequenceNumber(0)
{
}


void RTP_ControlFrame::AddTWCC(RTP_SyncSourceId syncSourceOut, const RTP_TransportWideCongestionControl & info)
{
  // Build the hideously complex format from https://tools.ietf.org/html/draft-holmer-rmcat-transport-wide-cc-extensions-01

  RTP_TransportWideCongestionControl::PacketMap::const_iterator itPkt = info.m_packets.begin();

  if (!PAssert(itPkt != info.m_packets.end(), PInvalidParameter))
    return;

  unsigned initialSN = itPkt->first;
  unsigned statusCount = info.m_packets.rbegin()->first - initialSN + 1;
  if (!PAssert(statusCount < 65536, PInvalidParameter))
    return;

  // Our reference time is the time of the first (lowest SN) packet, rounded down
  PTimeInterval referenceTime(itPkt->second.m_timestamp.GetMilliSeconds()/64*64);

  // Calculate the 250us quantised delta times between each packet, INT_MAX means missing.
  vector<int> quarterMillisecond(statusCount);
  quarterMillisecond[0] = (int)((itPkt->second.m_timestamp - referenceTime).GetMicroSeconds()/250);
  for (unsigned index = 1; index < statusCount; ++index)
    quarterMillisecond[index] = numeric_limits<int>::max();
  for (RTP_TransportWideCongestionControl::PacketMap::const_iterator prevPkt = itPkt++; itPkt != info.m_packets.end(); ++prevPkt,++itPkt)
    quarterMillisecond[itPkt->first - initialSN] = (int)((itPkt->second.m_timestamp - prevPkt->second.m_timestamp).GetMicroSeconds()/250);

  // Now calculate the status bits for each packet, either run length or matrix form
  vector<PUInt16b> statusChunks((statusCount+6)/7); // Worst case
  PUInt16b * statusPtr = statusChunks.data();
  unsigned runLength = 0;
  unsigned lastStatus = 0;
  for (unsigned index = 0; index < statusCount; ++index) {
    unsigned currentStatus = quarterMillisecond[index] < 0 ? 0 : (quarterMillisecond[index] < 256 ? 1 : 2);
    // Find a run length for same status, but stop and generate chunk on last packet
    if ((runLength == 0 || currentStatus == lastStatus) && index < statusCount - 1) {
      ++runLength;
      lastStatus = currentStatus; 
    }
    else {
      unsigned chunk;
      if (runLength >= 7) {
        if (currentStatus == lastStatus) // Edge case for last packet being in the run
          ++runLength;
        chunk = (lastStatus << 13) | runLength;  // Run length chunk
        --index;
      }
      else {
        // Vector chunk, we are not bothering with the 14x1 bit version for now.
        chunk = 3;
        for (unsigned run = 0; run < runLength; ++run)
          chunk = (chunk << 2) | lastStatus;
        chunk = (chunk << 2) | currentStatus;
        while (runLength < 6 && ++index < statusCount) {
          chunk = (chunk << 2) | (quarterMillisecond[index] < 0 ? 0 : (quarterMillisecond[index] < 256 ? 1 : 2));
          ++runLength;
        }
        while (runLength++ < 6)
          chunk <<= 2;
      }
      *statusPtr++ = (uint16_t)chunk;
      runLength = 0;
    }
  }
  size_t statusSize = (uint8_t *)statusPtr - (uint8_t *)statusChunks.data();

  // Then set the delta byte or word entries
  vector<uint8_t> deltas(statusCount*2); // Worst case is big for all
  uint8_t * deltaPtr = deltas.data();
  for (itPkt = info.m_packets.begin(); itPkt != info.m_packets.end(); ++itPkt) {
    int quarterMS = quarterMillisecond[itPkt->first - initialSN];
    if (quarterMS >= 0 && quarterMS < 256)
      *deltaPtr++ = (uint8_t)quarterMS;
    else if (quarterMS >= numeric_limits<int16_t>::min() && quarterMS <= numeric_limits<int16_t>::max()) {
      *(PInt16b *)deltaPtr = (int16_t)quarterMS;
      deltaPtr += 2;
    }
  }
  size_t deltaSize = deltaPtr - deltas.data();

  // Now we build the actual RTCP packet
  FbTWCC * twcc = (FbTWCC *)AddFeedback(e_TransportLayerFeedBack, e_TWCC, sizeof(FbTWCC) + statusSize + deltaSize);

  twcc->senderSSRC = syncSourceOut;
  twcc->mediaSSRC = 0;
  twcc->baseSN = (uint16_t)initialSN;
  twcc->statusCount = (uint16_t)statusCount;
  unsigned ms = (unsigned)referenceTime.GetMilliSeconds();
  twcc->referenceTime[0] = (uint8_t)(ms >> 21); // In 64ms increments
  twcc->referenceTime[1] = (uint8_t)(ms >> 13);
  twcc->referenceTime[2] = (uint8_t)(ms >> 5);
  twcc->rtcpSN = (uint8_t)info.m_rtcpSequenceNumber;
  memcpy(twcc+1, statusChunks.data(), statusSize);
  memcpy((BYTE *)(twcc+1)+statusSize, deltas.data(), deltaSize);
}


bool RTP_ControlFrame::ParseTWCC(RTP_SyncSourceId & senderSSRC, RTP_TransportWideCongestionControl & info)
{
  // Parse the hideously complex format from https://tools.ietf.org/html/draft-holmer-rmcat-transport-wide-cc-extensions-01

  info.m_packets.clear();

  size_t size = GetPayloadSize();
  if (size <= sizeof(FbTWCC))
    return false;

  // Get the basic bits of the packet timing info
  const FbTWCC * twcc = (const FbTWCC *)GetPayloadPtr();
  senderSSRC = twcc->senderSSRC;
  unsigned baseSN = twcc->baseSN;
  unsigned count = twcc->statusCount;
  PTimeInterval referenceTime((twcc->referenceTime[0] << 21U) | (twcc->referenceTime[1] << 13U) | (twcc->referenceTime[2] << 5U));
  info.m_rtcpSequenceNumber = twcc->rtcpSN;

  // Parse the status bits so we know if packet missing, small delta, or large delta
  vector<unsigned> status(count);
  unsigned index = 0;

  size -= sizeof(*twcc);
  const PUInt16b * chunks = (const PUInt16b *)(twcc+1);
  while (size >= 2 && index < count) {
    uint16_t chunk = *chunks++;

    if ((chunk & 0x8000) == 0) {
      // RUn length chunk, all the same
      unsigned runStatus = chunk >> 13;
      unsigned runLength = std::min(count, (chunk&0x1fffU));
      while (runLength-- > 0)
        status[index++] = runStatus;
    }
    else if ((chunk & 0x4000) == 0) {
      // single bit vector chunk, we assume "present" means small delta
      unsigned bits = 14;
      while (bits-- > 0 && index < count) {
        status[index++] = (chunk&0x2000) != 0;
        chunk <<= 1;
      }
    }
    else {
      // dual bit vector chunk
      unsigned bitPairs = 7;
      while (bitPairs-- > 0 && index < count) {
        status[index++] = (chunk&0x3000) >> 12;
        chunk <<= 2;
      }
    }
  }

  /* Now extract the delta times, based on the status bits extracted, and add
     the reference time so all times in PacketTimeMap are relative to whatever
     reference the remote is using. */
  const uint8_t * delta = (const uint8_t *)chunks;
  for (index = 0; index < count && size > 0; ++index) {
    int quarterMilliseconds;
    switch (status[index]) {
      case 1 : // Small delta
        quarterMilliseconds = *delta++ & 0xff;
        --size;
        break;

      case 2 : // Large delta
        quarterMilliseconds = *(PInt16b *)delta;
        delta += 2;
        size -= 2;
        break;

      default : // Absent
        continue;
    }

    referenceTime += PTimeInterval::MicroSeconds(quarterMilliseconds * 250U);
    info.m_packets.insert(make_pair(baseSN + index, referenceTime));
  }

  return !info.m_packets.empty();
}


void RTP_ControlFrame::AddPLI(RTP_SyncSourceId syncSourceOut, RTP_SyncSourceId syncSourceIn)
{
  FbHeader * hdr = AddFeedback(e_PayloadSpecificFeedBack, e_PictureLossIndication, sizeof(FbHeader));
  hdr->senderSSRC = syncSourceOut;
  hdr->mediaSSRC = syncSourceIn;
}


bool RTP_ControlFrame::ParsePLI(RTP_SyncSourceId & senderSSRC, RTP_SyncSourceId & targetSSRC)
{
  size_t size = GetPayloadSize();
  if (size < sizeof(FbHeader))
    return false;

  const FbHeader * hdr = (const FbHeader *)GetPayloadPtr();
  senderSSRC = hdr->senderSSRC;
  targetSSRC = hdr->mediaSSRC;
  return true;
}


void RTP_ControlFrame::AddFIR(RTP_SyncSourceId syncSourceOut, RTP_SyncSourceId syncSourceIn, unsigned sequenceNumber)
{
  FbFIR * fir;
  AddFeedback(e_PayloadSpecificFeedBack, e_FullIntraRequest, fir);

  fir->senderSSRC = syncSourceOut;
  fir->mediaSSRC = 0;
  fir->requestSSRC = syncSourceIn;
  fir->sequenceNumber = (BYTE)sequenceNumber;
}


bool RTP_ControlFrame::ParseFIR(RTP_SyncSourceId & senderSSRC, RTP_SyncSourceId & targetSSRC, unsigned & sequenceNumber)
{
  size_t size = GetPayloadSize();
  if (size < sizeof(FbFIR))
    return false;

  const FbFIR * fir = (const FbFIR *)GetPayloadPtr();
  senderSSRC = fir->senderSSRC;
  targetSSRC = fir->requestSSRC;
  sequenceNumber = fir->sequenceNumber;
  return true;
}


void RTP_ControlFrame::AddTSTO(RTP_SyncSourceId syncSourceOut, RTP_SyncSourceId syncSourceIn, unsigned tradeOff, unsigned sequenceNumber)
{
  FbTSTO * tsto;
  AddFeedback(e_PayloadSpecificFeedBack, e_TemporalSpatialTradeOffRequest, tsto);

  tsto->senderSSRC = syncSourceOut;
  tsto->mediaSSRC = 0;
  tsto->requestSSRC = syncSourceIn;
  tsto->sequenceNumber = (BYTE)sequenceNumber;
  tsto->tradeOff = (BYTE)tradeOff;
}


bool RTP_ControlFrame::ParseTSTO(RTP_SyncSourceId & senderSSRC, RTP_SyncSourceId & targetSSRC, unsigned & tradeOff, unsigned & sequenceNumber)
{
  size_t size = GetPayloadSize();
  if (size < sizeof(FbTSTO))
    return false;

  const FbTSTO * tsto = (const FbTSTO *)GetPayloadPtr();
  senderSSRC = tsto->senderSSRC;
  targetSSRC = tsto->requestSSRC;
  tradeOff = tsto->tradeOff;
  sequenceNumber = tsto->sequenceNumber;
  return true;
}


static const char REMB_ID[4] = { 'R', 'E', 'M', 'B' };

void RTP_ControlFrame::AddREMB(RTP_SyncSourceId syncSourceOut, RTP_SyncSourceId syncSourceIn, unsigned maxBitRate)
{
  FbREMB * remb;
  AddFeedback(e_PayloadSpecificFeedBack, e_ApplicationLayerFbMessage, remb);

  remb->senderSSRC = syncSourceOut;
  remb->mediaSSRC = 0;
  memcpy(remb->id, REMB_ID, 4);
  remb->numSSRC = 1;
  remb->feedbackSSRC[0] = syncSourceIn;

  unsigned exponent, mantissa;
  SplitBitRate(maxBitRate, exponent, mantissa, 1<<18);
  remb->bitRate[0] = (BYTE)((exponent << 2) | (mantissa >> 16));
  remb->bitRate[1] = (BYTE)(mantissa >> 8);
  remb->bitRate[2] = (BYTE) mantissa;
}


bool RTP_ControlFrame::ParseREMB(RTP_SyncSourceId & senderSSRC, RTP_SyncSourceArray & targetSSRCs, unsigned & maxBitRate)
{
  size_t size = GetPayloadSize();
  if (size < sizeof(FbREMB))
    return false;

  const FbREMB * remb = (const FbREMB *)GetPayloadPtr();
  if (memcmp(remb->id, REMB_ID, 4) != 0)
    return false;

  if (remb->numSSRC == 0)
    return false;
  if (remb->numSSRC > 1 && size < sizeof(FbREMB) + (remb->numSSRC - 1) * sizeof(remb->feedbackSSRC))
    return false;

  senderSSRC = remb->senderSSRC;
  targetSSRCs.resize(remb->numSSRC);
  for (int i = 0; i < remb->numSSRC; ++i)
      targetSSRCs[i] = remb->feedbackSSRC[i];
  maxBitRate = (((remb->bitRate[0]&0x3)<<16)|(remb->bitRate[1]<<8)|remb->bitRate[2])*(1 << (remb->bitRate[0] >> 2));
  return true;
}


RTP_ControlFrame::ApplDefinedInfo::ApplDefinedInfo(const char * type,
                                                   unsigned subType,
                                                   RTP_SyncSourceId ssrc,
                                                   const BYTE * data,
                                                   PINDEX size)
  : m_subType(subType)
  , m_SSRC(ssrc)
  , m_data(data, size)
{
  memset(m_type, 0, sizeof(m_type));
  if (type != NULL)
    strncpy(m_type, type, sizeof(m_type)-1);
}


void RTP_ControlFrame::AddApplDefined(const ApplDefinedInfo & info)
{
  StartNewPacket(e_ApplDefined);
  SetPayloadSize(info.m_data.GetSize() + 8);
  BYTE * payload = GetPayloadPtr();
  memcpy(payload + 4, info.m_type, 4);
  SetCount(info.m_subType);
  *(PUInt32b *)payload = info.m_SSRC;
  memcpy(payload + 8, info.m_data, info.m_data.GetSize());
  EndPacket();
}


bool RTP_ControlFrame::ParseApplDefined(ApplDefinedInfo & info)
{
  size_t size = GetPayloadSize();
  if (size < 8)
    return false;

  const BYTE * payload = GetPayloadPtr();

  memcpy(info.m_type, payload + 4, 4);
  info.m_type[4] = '\0';
  info.m_subType = GetCount();
  info.m_SSRC = *(const PUInt32b *)payload;
  info.m_data = PBYTEArray(payload + 8, size - 8);
  return true;
}


int RTP_ControlFrame::ReceiverReport::GetLostPackets() const
{
  // Use explicit 32 bit integer to make sign extension easier
  int32_t bits = (lost[0] << 16U) + (lost[1] << 8U) + lost[2];
  if (bits & 0x800000)
    bits |= 0xff000000;
  return bits;
}


void RTP_ControlFrame::ReceiverReport::SetLostPackets(int packets)
{
  if (packets < -8388608)
    packets = 0x800000;
  else if (packets > 0x7fffff)
    packets = 0x7fffff;

  lost[0] = (BYTE)(packets >> 16);
  lost[1] = (BYTE)(packets >> 8);
  lost[2] = (BYTE)packets;
}


/////////////////////////////////////////////////////////////////////////////

RTPHeaderExtensionInfo::RTPHeaderExtensionInfo()
  : m_id(0)
  , m_direction(Undefined)
{
}


RTPHeaderExtensionInfo::RTPHeaderExtensionInfo(const PURL & uri, const PString & attributes)
  : m_id(0)
  , m_direction(Undefined)
  , m_uri(uri)
  , m_attributes(attributes)
{
}


RTPHeaderExtensionInfo::RTPHeaderExtensionInfo(unsigned id, const PURL & uri, const PString & attributes)
  : m_id(id)
  , m_direction(Undefined)
  , m_uri(uri)
  , m_attributes(attributes)
{
}


bool RTPHeaderExtensions::AddUniqueID(RTPHeaderExtensionInfo & info)
{
  // Check URI not already present
  for (iterator it = begin(); it != end(); ++it) {
    if (it->m_uri == info.m_uri) {
      if (info.m_id == 0)
	info.m_id = it->m_id;
      return false;
    }
  }

  if (info.m_id == 0)
    info.m_id = 1; // Start here

  while (!insert(info).second)
    ++info.m_id;

  return true;
}


bool RTPHeaderExtensions::Contains(const RTPHeaderExtensionInfo & info) const
{
  if (info.m_id != 0) {
    const_iterator it = find(info);
    return it != end() && it->m_uri == info.m_uri;
  }

  for (const_iterator it = begin(); it != end(); ++it) {
    if (it->m_uri == info.m_uri)
      return true;
  }

  return false;
}

#if PTRACING
void RTPHeaderExtensionInfo::PrintOn(ostream & strm) const
{
  strm << "id=" << m_id << ", uri=" << m_uri;
  if (!m_attributes.IsEmpty())
    strm << ", attr=" << m_attributes;
}
#endif


PObject::Comparison RTPHeaderExtensionInfo::Compare(const PObject & obj) const
{
  return Compare2(m_id, dynamic_cast<const RTPHeaderExtensionInfo &>(obj).m_id);
}


#if OPAL_SDP
bool RTPHeaderExtensionInfo::ParseSDP(const PString & param)
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


void RTPHeaderExtensionInfo::OutputSDP(ostream & strm) const
{
  strm << "a=extmap:" << m_id;

  switch (m_direction) {
    case RTPHeaderExtensionInfo::Inactive :
      strm << "/inactive";
      break;
    case RTPHeaderExtensionInfo::SendOnly :
      strm << "/sendonly";
      break;
    case RTPHeaderExtensionInfo::RecvOnly :
      strm << "/recvonly";
      break;
    case RTPHeaderExtensionInfo::SendRecv :
      strm << "/sendrecv";
      break;
    default :
      break;
  }

  strm << ' ' << m_uri;
  if (!m_attributes.IsEmpty())
    strm << ' ' << m_attributes;

  strm << "\r\n";
}
#endif // OPAL_SDP

    
/////////////////////////////////////////////////////////////////////////////
