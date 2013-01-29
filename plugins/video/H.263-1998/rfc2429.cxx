/*
 * H.264 Plugin codec for OpenH323/OPAL
 *
 * Copyright (C) Matthias Schneider, All Rights Reserved
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
 * Contributor(s): Matthias Schneider (ma30002000@yahoo.de)
 *
 *
 */

#include "rfc2429.h"

#include <codec/opalplugin.hpp>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <cmath>


static const char formats[8][64] = { "forbidden",
                                     "sub-QCIF",
                                     "QCIF",
                                     "CIF",
                                     "4CIF",
                                     "16CIF",
                                     "reserved",
                                     "extended PTYPE" };

static const char picTypeCodes[8][64] = { "I-Picture",
                                          "P-Picture",
                                          "improved PB-frame",
                                          "B-Picture",
                                          "EI-Picture",
                                          "EP-Picture",
                                          "reserved (110)",
                                          "reserved (111)" };

static const char plusFormats[8][64] = { "forbidden",
                                         "sub-QCIF",
                                         "QCIF",
                                         "CIF",
                                         "4CIF",
                                         "16CIF",
                                         "custom format",
                                         "reserved" };

static const char PARs[16][64]   = { "forbidden",
                                     "1:1 (Square)",
                                     "12:11 (CIF for 4:3 picture)",
                                     "10:11 (525-type for 4:3 picture)",
                                     "16:11 (CIF stretched for 16:9 picture)",
                                     "40:33 (525-type stretched for 16:9 picture)",
                                     "reserved (0110)",
                                     "reserved (0111)",
                                     "reserved (1000)",
                                     "reserved (1001)",
                                     "reserved (1010)",
                                     "reserved (1011)",
                                     "reserved (1100)",
                                     "reserved (1101)",
                                     "reserved (1110)",
                                     "Extended PAR" };



Bitstream::Bitstream ()
{
  m_data.ptr = NULL;
}

void Bitstream::SetBytes (uint8_t* data, size_t dataLen, uint8_t sbits, uint8_t ebits)
{
  m_data.ptr = data;
  m_data.len = dataLen;
  m_data.pos = sbits;
  m_sbits = sbits;
  m_ebits = ebits;
}

void Bitstream::GetBytes (uint8_t** data, size_t * dataLen)
{
  *data = m_data.ptr;
  *dataLen = m_data.len;
}

uint32_t Bitstream::GetBits(uint32_t numBits)
{
  uint32_t ret;
  ret = PeekBits(numBits);
  m_data.pos += numBits;
  return (ret);
}

uint32_t Bitstream::PeekBits(uint32_t numBits)
{
    uint8_t i;
    uint32_t result = 0;
    uint32_t offset = m_data.pos / 8;
    uint8_t  offsetBits = (uint8_t)(m_data.pos % 8);
    if (((m_data.len << 3) - m_ebits - m_sbits) < (m_data.pos  + numBits)) {
      PTRACE(2, "RFC2429",
                "Frame too short, trying to read " << numBits << " bits at position " << m_data.pos <<
                " when frame is only " << ((m_data.len << 3) - m_ebits - m_sbits) << " bits long");
      return 0;
    }

    for (i=0; i < numBits; i++) {
        result = result << 1;
        switch (offsetBits) {
            case 0: if ((m_data.ptr[offset] & 0x80)>>7) result = result | 0x01; break;
            case 1: if ((m_data.ptr[offset] & 0x40)>>6) result = result | 0x01; break;
            case 2: if ((m_data.ptr[offset] & 0x20)>>5) result = result | 0x01; break;
            case 3: if ((m_data.ptr[offset] & 0x10)>>4) result = result | 0x01; break;
            case 4: if ((m_data.ptr[offset] & 0x08)>>3) result = result | 0x01; break;
            case 5: if ((m_data.ptr[offset] & 0x04)>>2) result = result | 0x01; break;
            case 6: if ((m_data.ptr[offset] & 0x02)>>1) result = result | 0x01; break;
            case 7: if ( m_data.ptr[offset] & 0x01)     result = result | 0x01; break;
        }
        offsetBits++;
        if (offsetBits>7) {offset++; offsetBits=0;}
    }
    return (result);
};

void Bitstream::PutBits(uint32_t posBits, uint32_t numBits, uint32_t value)
{
  uint8_t i;
  posBits += m_sbits;
  uint32_t offset = m_data.pos / 8;
  uint8_t  offsetBits = (uint8_t)(m_data.pos % 8);
  static const uint8_t maskClear[8] = {
    0x7f, 0xbf, 0xdf, 0xef,
    0xf7, 0xfb, 0xfd, 0xfe
  };

  static const uint8_t maskSet[8] = {
    0x80, 0x40, 0x20, 0x10,
    0x08, 0x04, 0x02, 0x01
  };

  for (i=0; i < numBits; i++) {
    if (value & (1<<(numBits-i-1)))
      m_data.ptr[offset] |= maskSet[offsetBits];
    else
      m_data.ptr[offset] &= maskClear[offsetBits];
    offsetBits++;
    if (offsetBits > 7) {
      offset++;
      offsetBits=0;
    }
  }
};

void Bitstream::SetPos(uint32_t pos)
{
  m_data.pos = pos;
}

uint32_t Bitstream::GetPos()
{
  return (m_data.pos);
}

RFC2429Frame::RFC2429Frame()
  : m_packetizationOffset(0)
  , m_minPayloadSize(0)
  , m_customClock(false)
{
}


bool RFC2429Frame::GetPacket(PluginCodec_RTP & frame, unsigned int & flags)
{
  if (m_buffer == NULL || m_packetizationOffset >= m_length)
    return false;

  // this is the first packet of a new frame
  // we parse the frame for SCs 
  // and later try to split into packets at these borders
  if (m_packetizationOffset == 0) {   
    m_startCodes.clear();          
    for (size_t i = 1; i < m_length; ++i) {
      if (m_buffer[i-1] == 0 && m_buffer[i] == 0)
        m_startCodes.push_back(i);
    }  
    unsigned requiredPackets = (m_length+m_maxPayloadSize-1)/m_maxPayloadSize;
    if (m_length > m_maxPayloadSize)
      m_minPayloadSize = m_length/requiredPackets;
    else
      m_minPayloadSize = m_length;
    PTRACE(6, GetName(),
              "Setting minimal packet size to " << m_minPayloadSize <<
              " considering " << requiredPackets << 
              " packets for this frame");
  }

  bool hasStartCode = false;
  uint8_t* payloadPtr = frame.GetPayloadPtr();  

  // RFC 2429 / RFC 4629 header
  // no extra header, no VRC
  // we do try to save the 2 bytes of the PSC though
  payloadPtr [0] = 0;
  if ((m_buffer[m_packetizationOffset] == 0) &&
      (m_buffer[m_packetizationOffset+1] == 0)) {
    payloadPtr[0] |= 0x04;
    m_packetizationOffset +=2;
  }
  payloadPtr [1] = 0;

  // skip all start codes below m_minPayloadSize
  while ((!m_startCodes.empty()) && (m_startCodes.front() < m_minPayloadSize)) {
    hasStartCode = true;
    m_startCodes.erase(m_startCodes.begin());
  }

  // if there is a startcode between m_minPayloadSize and m_maxPayloadSize set 
  // the packet boundary there, if not, use m_maxPayloadSize
  if ((!m_startCodes.empty()) 
   && ((m_startCodes.front() - m_packetizationOffset) > m_minPayloadSize)
   && ((m_startCodes.front() - m_packetizationOffset) < (unsigned)(m_maxPayloadSize - 2))) {
    frame.SetPayloadSize(m_startCodes.front() - m_packetizationOffset + 2);
    m_startCodes.erase(m_startCodes.begin());
  }
  else {
    size_t payloadSize = m_length - m_packetizationOffset + 2;
    if (payloadSize > m_maxPayloadSize)
      payloadSize = m_maxPayloadSize;
    frame.SetPayloadSize(payloadSize);
  }
  PTRACE(6, GetName(), "Sending "<< (frame.GetPayloadSize() - 2) <<" bytes at position " << m_packetizationOffset);
  memcpy(payloadPtr + 2, m_buffer + m_packetizationOffset, frame.GetPayloadSize() - 2);
  m_packetizationOffset += frame.GetPayloadSize() - 2;

  if (m_length == m_packetizationOffset)
    flags |= PluginCodec_ReturnCoderLastFrame;

  return true;
}


bool RFC2429Frame::AddPacket(const PluginCodec_RTP & packet, unsigned int & flags)
{
  size_t payloadSize = packet.GetPayloadSize();
  if (payloadSize < 3) {
    PTRACE(2, GetName(), "Packet too short (<3)");
    flags |= PluginCodec_ReturnCoderRequestIFrame;
    return true;
  }

  uint8_t* payloadPtr = packet.GetPayloadPtr();
  bool headerP = (payloadPtr[0] & 0x04) != 0;
  bool headerV = (payloadPtr[0] & 0x02) != 0;
  unsigned headerPLEN = ((payloadPtr[0] & 0x01) << 5) + ((payloadPtr[1] & 0xF8) >> 3);
  unsigned headerPEBITS = (payloadPtr[1] & 0x07);
  payloadPtr += 2;

  PTRACE(6, GetName(),
            "RFC 2429 Header: P: "     << headerP
                         << " V: "     << headerV
                         << " PLEN: "  << headerPLEN
                         << " PBITS: " << headerPEBITS);

  if (headerV)
    payloadPtr++; // We ignore the VRC

  if (headerPLEN > 0) {
    if (payloadSize < headerPLEN + (headerV ? 3U : 2U)) {
      PTRACE(2, GetName(), "Packet too short (header)");
      flags |= PluginCodec_ReturnCoderRequestIFrame;
      return true;
    }
    // we store the extra header for now, but dont do anything with it right now
    memcpy(m_picHeader.buf + 2, payloadPtr, headerPLEN);
    m_picHeader.len = headerPLEN + 2;
    m_picHeader.pebits = headerPEBITS;
    payloadPtr += headerPLEN;
  }

  unsigned remBytes = payloadSize - headerPLEN - (headerV ? 3 : 2);

  if (headerP) {
    PTRACE(6, GetName(), "Adding startcode of 2 bytes to frame of " << remBytes << " bytes");
    static uint8_t startcode[2];
    if (!Append(startcode, sizeof(startcode)))
      return false;
  }

  PTRACE(6, GetName(), "Adding " << remBytes << " bytes to frame of " << m_packetizationOffset << " bytes");
  if (!Append(payloadPtr, remBytes))
    return false;

  if (packet.GetMarker())  { 
    if (headerP && (payloadPtr[0] & 0xfc) == 0x80) {
      size_t hdrLen = ParseHeader(payloadPtr + (headerP ? 0 : 2), payloadSize - 2 - (headerP ? 0 : 2));
      PTRACE(6, GetName(), "Frame includes a picture header of " << hdrLen << " bits");
    }
    else {
      PTRACE(3, GetName(), "Frame does not seem to include a picture header");
    }
  }

  return true;
}


bool RFC2429Frame::Reset(size_t len)
{
  m_packetizationOffset = 0;
  m_picHeader.len = 0;
  m_customClock = false;
  m_startCodes.clear();

  return FFMPEGCodec::EncodedFrame::Reset(len);
}


bool RFC2429Frame::IsIntraFrame() const
{
  Bitstream headerBits;
  headerBits.SetBytes (m_buffer, m_length, 0, 0);
  headerBits.SetPos(35);
  if (headerBits.GetBits(3) == 7) { // This is the plustype
    if (headerBits.GetBits(3) == 1)
      headerBits.SetPos(59);
    return headerBits.GetBits(3) == 0;
  }

  headerBits.SetPos(26);
  return headerBits.GetBits(1) == 0;
}


size_t RFC2429Frame::ParseHeader(uint8_t* headerPtr, size_t headerMaxLen) 
{
  Bitstream headerBits;
  headerBits.SetBytes (headerPtr, headerMaxLen, 0, 0);

  unsigned PTYPEFormat = 0;
  unsigned PLUSPTYPEFormat = 0;
  unsigned PTCODE = 0;
  unsigned UFEP = 0;
  
  bool PB  = false;
  bool CPM = false;
  //bool PEI = false;
  bool SS  = false;
  bool RPS = false;
  bool PCF = false;
  bool UMV = false;

  headerBits.SetPos(6);
  PTRACE(6, GetName(), "Header\tTR:" << headerBits.GetBits(8));                        // TR
  headerBits.GetBits(2);                                                          // PTYPE, skip 1 0 bits
  PTRACE(6, GetName(), "Header\tSplit Screen: "    << headerBits.GetBits(1) 
                         << " Document Camera: " << headerBits.GetBits(1)
                         << " Picture Freeze: "  << headerBits.GetBits(1));
  PTYPEFormat = headerBits.GetBits(3); 
  if (PTYPEFormat == 7) { // This is the plustype
    UFEP = headerBits.GetBits(3);                                                 // PLUSPTYPE
    if (UFEP==1) {                                                                // OPPTYPE
      PLUSPTYPEFormat = headerBits.GetBits(3);
      PTRACE(6, GetName(), "Header\tPicture: " << formats[PTYPEFormat] << ", "<< (plusFormats [PLUSPTYPEFormat]));
      PCF = headerBits.GetBits(1) != 0;
      UMV = headerBits.GetBits(1) != 0;
      m_customClock = PCF;
      PTRACE(6, GetName(), "Header\tPCF: " << PCF 
                             << " UMV: " << UMV 
                             << " SAC: " << headerBits.GetBits(1) 
                             << " AP: "  << headerBits.GetBits(1) 
                             << " AIC: " << headerBits.GetBits(1)
                             << " DF: "  << headerBits.GetBits(1));
      SS = headerBits.GetBits(1) != 0;
      RPS = headerBits.GetBits(1) != 0;
      PTRACE(6, GetName(), "Header\tSS: "  << SS
                          << " RPS: " << RPS
                          << " ISD: " << headerBits.GetBits(1)
                          << " AIV: " << headerBits.GetBits(1)
                          << " MQ: "  << headerBits.GetBits(1));
      headerBits.GetBits(4);                                                      // skip 1 0 0 0
    }
    PTCODE = headerBits.GetBits(3);
    if (PTCODE == 2) PB = true;
    PTRACE(6, GetName(), "Header\tPicture: " << picTypeCodes [PTCODE]                  // MPPTYPE
                           << " RPR: " << headerBits.GetBits(1) 
                           << " RRU: " << headerBits.GetBits(1) 
                        << " RTYPE: " << headerBits.GetBits(1));
    headerBits.GetBits(3);                                                        // skip 0 0 1

    CPM = headerBits.GetBits(1) != 0;                                                  // CPM + PSBI
    if (CPM) {
      PTRACE(6, GetName(), "Header\tCPM: " << CPM << " PSBI: " << headerBits.GetBits(2));
    } else {
      PTRACE(6, GetName(), "Header\tCPM: " << CPM );
    }
    if (UFEP == 1) {
      if (PLUSPTYPEFormat == 6) {

        uint32_t PAR, width, height;

        PAR = headerBits.GetBits(4);                                              // PAR
        width = (headerBits.GetBits(9) + 1) * 4;                                  // PWI
        headerBits.GetBits(1);                                                    // skip 1
        height = (headerBits.GetBits(9) + 1) * 4;                                 // PHI
        PTRACE(6, GetName(), "Header\tAspect Ratio: " << (PARs [PAR]) << 
                                  " Resolution: " << width << "x" <<  height);

        if (PAR == 15) {                                                          // EPAR
          PTRACE(6, GetName(), "Header\tExtended Aspect Ratio: " 
                 << headerBits.GetBits(8) << "x" <<  headerBits.GetBits(8));
        } 
      }
      if (PCF) {
        PTRACE(6, GetName(), "Header\tCustom Picture Clock Frequency "
			   << (1800000 / (headerBits.GetBits(7) * (headerBits.GetBits(1) + 1000.0))));
      }
    }

    if (m_customClock) {
      PTRACE(6, GetName(), "Header\tETR: " << headerBits.GetBits(2));
    }

    if (UFEP == 1) {
      if (UMV)  {
         if (headerBits.GetBits(1) == 1)
           PTRACE(6, GetName(), "Header\tUUI: 1");
          else
           PTRACE(6, GetName(), "Header\tUUI: 0" << headerBits.GetBits(1));
      }
      if (SS) {
        PTRACE(6, GetName(), "Header\tSSS:" << headerBits.GetBits(2));
      }
    }

    if ((PTCODE==3) || (PTCODE==4) || (PTCODE==5)) {
      PTRACE(6, GetName(), "Header\tELNUM: " << headerBits.GetBits(4));
      if (UFEP==1)
        PTRACE(6, GetName(), "Header\tRLNUM: " << headerBits.GetBits(4));
    }

    if (RPS) {
        PTRACE(1, GetName(), "Header\tDecoding of RPS parameters not supported");
        return 0;
    }  
    PTRACE(6, GetName(), "Header\tPQUANT: " << headerBits.GetBits(5));                    // PQUANT
  } else {
    PTRACE(6, GetName(), "Header\tPicture: " << formats[PTYPEFormat] 
                                   << ", " << (headerBits.GetBits(1) ? "P-Picture" : "I-Picture")  // still PTYPE
                               << " UMV: " << headerBits.GetBits(1) 
                               << " SAC: " << headerBits.GetBits(1) 
                               << " APC: " << headerBits.GetBits(1));
    PB = headerBits.GetBits(1) != 0;                                                      // PB
    PTRACE(6, GetName(), "Header\tPB-Frames: " << PB);
    PTRACE(6, GetName(), "Header\tPQUANT: " << headerBits.GetBits(5));                    // PQUANT

    CPM = headerBits.GetBits(1) != 0;
    if (CPM) {
      PTRACE(6, GetName(), "Header\tCPM: " << CPM << " PSBI: " << headerBits.GetBits(2)); // CPM + PSBI
    } else {
      PTRACE(6, GetName(), "Header\tCPM: " << CPM ); 
    }
  }	

  if (PB)
    PTRACE(6, GetName(), "Header\tTRB: " << headerBits.GetBits (3)                     // TRB
                    << " DBQUANT: " << headerBits.GetBits (2));                   // DQUANT

  while (headerBits.GetBits (1)) {                                                // PEI bit
    PTRACE(6, GetName(), "Header\tPSUPP: " << headerBits.GetBits (8));                 // PSPARE bits 
  }

  return headerBits.GetPos();
} 
