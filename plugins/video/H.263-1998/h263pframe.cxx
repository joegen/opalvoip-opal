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

#include "h263pframe.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <cmath>
#define MAX_HEADER_SIZE 255

Bitstream::Bitstream ()
{
  _data.ptr = NULL;
}

void Bitstream::SetBytes (uint8_t* data, uint32_t dataLen, uint8_t sbits, uint8_t ebits)
{
  _data.ptr = data;
  _data.len = dataLen;
  _data.pos = sbits;
  _sbits = sbits;
  _ebits = ebits;
}

void Bitstream::GetBytes (uint8_t** data, uint32_t * dataLen)
{
  *data = _data.ptr;
  *dataLen = _data.len;
}

uint32_t Bitstream::GetBits(uint32_t numBits) {
  uint32_t ret;
  ret = PeekBits(numBits);
  _data.pos += numBits;
  return (ret);
}

uint32_t Bitstream::PeekBits(uint32_t numBits) {
    uint8_t i;
    uint32_t result = 0;
    uint32_t offset = _data.pos / 8;
    uint8_t  offsetBits = (uint8_t)(_data.pos % 8);
    if (((_data.len << 3) -_ebits - _sbits) < (_data.pos  + numBits)) {
      PTRACE(1, "H263+", "Deencap\tFrame too short, trying to read " << numBits << " bits at position " << _data.pos 
                           << " when frame is only " << ((_data.len << 3) -_ebits - _sbits) << " bits long");
      return 0;
    }

    for (i=0; i < numBits; i++) {
        result = result << 1;
        switch (offsetBits) {
            case 0: if ((_data.ptr[offset] & 0x80)>>7) result = result | 0x01; break;
            case 1: if ((_data.ptr[offset] & 0x40)>>6) result = result | 0x01; break;
            case 2: if ((_data.ptr[offset] & 0x20)>>5) result = result | 0x01; break;
            case 3: if ((_data.ptr[offset] & 0x10)>>4) result = result | 0x01; break;
            case 4: if ((_data.ptr[offset] & 0x08)>>3) result = result | 0x01; break;
            case 5: if ((_data.ptr[offset] & 0x04)>>2) result = result | 0x01; break;
            case 6: if ((_data.ptr[offset] & 0x02)>>1) result = result | 0x01; break;
            case 7: if ( _data.ptr[offset] & 0x01)     result = result | 0x01; break;
        }
        offsetBits++;
        if (offsetBits>7) {offset++; offsetBits=0;}
    }
    return (result);
};

void Bitstream::PutBits(uint32_t posBits, uint32_t numBits, uint32_t value) {
    uint8_t i;
    posBits += _sbits;
    uint32_t offset = _data.pos / 8;
    uint8_t  offsetBits = (uint8_t)(_data.pos % 8);
    static const uint8_t maskClear[8] = {
      0x7f, 0xbf, 0xdf, 0xef,
      0xf7, 0xfb, 0xfd, 0xfe
    };

    static const uint8_t maskSet[8] = {
      0x80, 0x40, 0x20, 0x10,
      0x08, 0x04, 0x02, 0x01
    };

    for (i=0; i < numBits; i++) {
        if (value & (1<<(numBits-i-1))) {
            _data.ptr[offset] |= maskSet[offsetBits];}
          else {
            _data.ptr[offset] &= maskClear[offsetBits];}
        offsetBits++;
        if (offsetBits > 7) {offset++; offsetBits=0;}
    }
};

void Bitstream::SetPos(uint32_t pos) {
  _data.pos = pos;
}

uint32_t Bitstream::GetPos() {
  return (_data.pos);
}

H263PFrame::H263PFrame (uint32_t maxFrameSize)
{
  _timestamp = 0;
  _maxPayloadSize = 1400;
  _maxFrameSize = maxFrameSize;

  _encodedFrame.ptr = (uint8_t*) malloc(maxFrameSize);
  _picHeader.ptr = (uint8_t*) malloc(MAX_HEADER_SIZE);
  BeginNewFrame();
}

H263PFrame::~H263PFrame ()
{
  if (_encodedFrame.ptr) free (_encodedFrame.ptr);
  if (_picHeader.ptr) free (_picHeader.ptr);
}

void H263PFrame::BeginNewFrame ()
{
  _encodedFrame.len = 0;
  _encodedFrame.pos = 0;
  _picHeader.len = 0;
  _customClock = false;
}

void H263PFrame::GetRTPFrame (PluginCodec_RTP & frame, unsigned int & flags)
{
  uint32_t i;
  // this is the first packet of a new frame
  // we parse the frame for SCs 
  // and later try to split into packets at these borders
  if (_encodedFrame.pos == 0) {   
    _startCodes.clear();          
    for (i=0; i < _encodedFrame.len - 1; i++ ) {
      if ((_encodedFrame.ptr[i] == 0) && (_encodedFrame.ptr[i+1]==0)) {
        _startCodes.push_back(i);
      }
    }  
    if (_encodedFrame.len > _maxPayloadSize)
      _minPayloadSize = (uint16_t)(_encodedFrame.len / ceil((float)_encodedFrame.len / (float)_maxPayloadSize));
    else
      _minPayloadSize = (uint16_t)_encodedFrame.len;
    PTRACE(4, "H263+", "Encap\tSetting minimal packet size to " << _minPayloadSize
          << " considering " << ceil((float)_encodedFrame.len / (float)_maxPayloadSize) << " packets for this frame");
  }

  bool hasStartCode = false;
  uint8_t* dataPtr = frame.GetPayloadPtr();  

  // RFC 2429 / RFC 4629 header
  // no extra header, no VRC
  // we do try to save the 2 bytes of the PSC though
  dataPtr [0] = 0;
  if ((_encodedFrame.ptr[_encodedFrame.pos] == 0) 
   && (_encodedFrame.ptr[_encodedFrame.pos + 1] == 0)) {
    dataPtr[0] |= 0x04;
    _encodedFrame.pos +=2;
  }
  dataPtr [1] = 0;

  // skip all start codes below _minPayloadSize
  while ((!_startCodes.empty()) && (_startCodes.front() < _minPayloadSize)) {
    hasStartCode = true;
    _startCodes.erase(_startCodes.begin());
  }

  // if there is a startcode between _minPayloadSize and _maxPayloadSize set 
  // the packet boundary there, if not, use _maxPayloadSize
  if ((!_startCodes.empty()) 
   && ((_startCodes.front() - _encodedFrame.pos) > _minPayloadSize)
   && ((_startCodes.front() - _encodedFrame.pos) < (unsigned)(_maxPayloadSize - 2))) {
    frame.SetPayloadSize(_startCodes.front() - _encodedFrame.pos + 2);
    _startCodes.erase(_startCodes.begin());
  }
  else {
    if (_encodedFrame.pos + (_maxPayloadSize - 2) <= _encodedFrame.len)
      frame.SetPayloadSize(_maxPayloadSize);
     else
      frame.SetPayloadSize(_encodedFrame.len - _encodedFrame.pos + 2 );
  }
  PTRACE(4, "H263+", "Encap\tSending "<< (frame.GetPayloadSize() - 2) <<" bytes at position " << _encodedFrame.pos);
  memcpy(frame.GetPayloadPtr() + 2, _encodedFrame.ptr + _encodedFrame.pos, frame.GetPayloadSize() - 2);
  _encodedFrame.pos += frame.GetPayloadSize() - 2;

  frame.SetTimestamp((unsigned long)_timestamp);
  if (_encodedFrame.len == _encodedFrame.pos)  
     frame.SetMarker(1);
    else
     frame.SetMarker(0);

  flags = 0;
  flags |= frame.GetMarker() ? isLastFrame : 0;
  flags |= IsIFrame() ? isIFrame : 0;
}

bool H263PFrame::SetFromRTPFrame(PluginCodec_RTP & frame, unsigned int & /*flags*/)
{

  if (frame.GetPayloadSize()<3) {
    PTRACE(1, "H263+", "Deencap\tFrame too short (<3)");
    return false;
  }

  uint8_t* dataPtr = frame.GetPayloadPtr();
  uint8_t headerP = dataPtr[0] & 0x04;
  uint8_t headerV = dataPtr[0] & 0x02;
  uint8_t headerPLEN = ((dataPtr[0] & 0x01) << 5) + ((dataPtr[1] & 0xF8) >> 3);
  uint8_t headerPEBITS = (dataPtr[1] & 0x07);
  uint32_t remBytes;
  dataPtr += 2;

  PTRACE(4, "H263+", "Deencap\tRFC 2429 Header: P: "     << (headerP ? 1 : 0)
                                           << " V: "     << (headerV ? 1 : 0)
                                           << " PLEN: "  << (uint32_t) headerPLEN
                                           << " PBITS: " << (uint32_t) headerPEBITS);

  if (headerV) dataPtr++; // We ignore the VRC
  if (headerPLEN > 0) {
    if (frame.GetPayloadSize() < headerPLEN + (headerV ? 3U : 2U)) {
      PTRACE(1, "H263+", "Deencap\tFrame too short (header)");
      return false;
    }
    // we store the extra header for now, but dont do anything with it right now
    memcpy(_picHeader.ptr + 2, dataPtr, headerPLEN);
    _picHeader.len = headerPLEN + 2;
    _picHeader.pebits = headerPEBITS;
    dataPtr += headerPLEN;
  }

  remBytes = frame.GetPayloadSize() - headerPLEN - (headerV ? 3 : 2);

  if ((_encodedFrame.pos + (headerP ? 2 : 0) + remBytes) > (_maxFrameSize - FF_INPUT_BUFFER_PADDING_SIZE)) {
    PTRACE(1, "H263+", "Deencap\tTrying to add " << remBytes 
         << " bytes to frame at position " << _encodedFrame.pos + (headerP ? 2 : 0) 
         << " bytes while maximum frame size is  " << _maxFrameSize << "-" << FF_INPUT_BUFFER_PADDING_SIZE << " bytes");
    return false;
  }


  if (headerP) {
    PTRACE(4, "H263+", "Deencap\tAdding startcode of 2 bytes to frame of " << remBytes << " bytes");
    memset (_encodedFrame.ptr + _encodedFrame.pos, 0, 2);
    _encodedFrame.pos +=2;
    _encodedFrame.len +=2;
  }

  PTRACE(4, "H263+", "Deencap\tAdding " << remBytes << " bytes to frame of " << _encodedFrame.pos << " bytes");
  memcpy(_encodedFrame.ptr + _encodedFrame.pos, dataPtr, remBytes);
  _encodedFrame.pos += remBytes;
  _encodedFrame.len += remBytes;

  if (frame.GetMarker())  { 
    if ((headerP) && ((dataPtr[(headerP ? 0 : 2)] & 0xfc) == 0x80)) {
      uint32_t hdrLen = parseHeader(dataPtr + (headerP ? 0 : 2), frame.GetPayloadSize()- 2 - (headerP ? 0 : 2));
      PTRACE(4, "H263+", "Deencap\tFrame includes a picture header of " << hdrLen << " bits");
    }
    else {
      PTRACE(1, "H263+", "Deencap\tFrame does not seem to include a picture header");
    }
  }

  return true;
}

bool H263PFrame::hasPicHeader () {
  Bitstream headerBits;
  headerBits.SetBytes (_encodedFrame.ptr, _encodedFrame.len, 0, 0);
  if ((headerBits.GetBits(16) == 0) && (headerBits.GetBits(6) == 32)) 
    return true;
  return false;
}

bool H263PFrame::IsIFrame () {
  Bitstream headerBits;
  if (hasPicHeader())
    headerBits.SetBytes (_encodedFrame.ptr, _encodedFrame.len, 0, 0);
   else
    return 0; // to be implemented
  headerBits.SetPos(35);
  uint32_t PTYPEFormat = headerBits.GetBits(3); 
  if (PTYPEFormat == 7) { // This is the plustype
    uint32_t UFEP = headerBits.GetBits(3); 
    if (UFEP == 1) {
        headerBits.SetPos(59);
        return (headerBits.GetBits(3) == 0);
     } else 
        return (headerBits.GetBits(3) == 0);
  }
  else {
    headerBits.SetPos(26);
    return (headerBits.GetBit() == 0);
  }
}

uint32_t H263PFrame::parseHeader(uint8_t* headerPtr, uint32_t headerMaxLen) 
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
  PTRACE(4, "H263+", "Header\tTR:" << headerBits.GetBits(8));                        // TR
  headerBits.GetBits(2);                                                          // PTYPE, skip 1 0 bits
  PTRACE(4, "H263+", "Header\tSplit Screen: "    << headerBits.GetBit() 
                         << " Document Camera: " << headerBits.GetBit()
                         << " Picture Freeze: "  << headerBits.GetBit());
  PTYPEFormat = headerBits.GetBits(3); 
  if (PTYPEFormat == 7) { // This is the plustype
    UFEP = headerBits.GetBits(3);                                                 // PLUSPTYPE
    if (UFEP==1) {                                                                // OPPTYPE
      PLUSPTYPEFormat = headerBits.GetBits(3);
      PTRACE(4, "H263+", "Header\tPicture: " << formats[PTYPEFormat] << ", "<< (plusFormats [PLUSPTYPEFormat]));
      PCF = headerBits.GetBit();
      UMV = headerBits.GetBit();
      _customClock = PCF;
      PTRACE(4, "H263+", "Header\tPCF: " << PCF 
                             << " UMV: " << UMV 
                             << " SAC: " << headerBits.GetBit() 
                             << " AP: "  << headerBits.GetBit() 
                             << " AIC: " << headerBits.GetBit()
                             << " DF: "  << headerBits.GetBit());
      SS = headerBits.GetBit();
      RPS = headerBits.GetBit();
      PTRACE(4, "H263+", "Header\tSS: "  << SS
                          << " RPS: " << RPS
                          << " ISD: " << headerBits.GetBit()
                          << " AIV: " << headerBits.GetBit()
                          << " MQ: "  << headerBits.GetBit());
      headerBits.GetBits(4);                                                      // skip 1 0 0 0
    }
    PTCODE = headerBits.GetBits(3);
    if (PTCODE == 2) PB = true;
    PTRACE(4, "H263+", "Header\tPicture: " << picTypeCodes [PTCODE]                  // MPPTYPE
                           << " RPR: " << headerBits.GetBit() 
                           << " RRU: " << headerBits.GetBit() 
                        << " RTYPE: " << headerBits.GetBit());
    headerBits.GetBits(3);                                                        // skip 0 0 1

    CPM = headerBits.GetBit();                                                  // CPM + PSBI
    if (CPM) {
      PTRACE(4, "H263+", "Header\tCPM: " << CPM << " PSBI: " << headerBits.GetBits(2));
    } else {
      PTRACE(4, "H263+", "Header\tCPM: " << CPM );
    }
    if (UFEP == 1) {
      if (PLUSPTYPEFormat == 6) {

        uint32_t PAR, width, height;

        PAR = headerBits.GetBits(4);                                              // PAR
        width = (headerBits.GetBits(9) + 1) * 4;                                  // PWI
        headerBits.GetBit();                                                    // skip 1
        height = (headerBits.GetBits(9) + 1) * 4;                                 // PHI
        PTRACE(4, "H263+", "Header\tAspect Ratio: " << (PARs [PAR]) << 
                                  " Resolution: " << width << "x" <<  height);

        if (PAR == 15) {                                                          // EPAR
          PTRACE(4, "H263+", "Header\tExtended Aspect Ratio: " 
                 << headerBits.GetBits(8) << "x" <<  headerBits.GetBits(8));
        } 
      }
      if (PCF) {

        double factor, divisor, freq;

        factor = headerBits.GetBit() + 1000;
        divisor = headerBits.GetBits(7);
        freq = 1800000 / (divisor * factor);

        PTRACE(4, "H263+", "Header\tCustom Picture Clock Frequency " << freq);
      }
    }

    if (_customClock) {
      PTRACE(4, "H263+", "Header\tETR: " << headerBits.GetBits(2));
    }

    if (UFEP == 1) {
      if (UMV)  {
         if (headerBits.GetBit() == 1)
           PTRACE(4, "H263+", "Header\tUUI: 1");
          else
           PTRACE(4, "H263+", "Header\tUUI: 0" << headerBits.GetBit());
      }
      if (SS) {
        PTRACE(4, "H263+", "Header\tSSS:" << headerBits.GetBits(2));
      }
    }

    if ((PTCODE==3) || (PTCODE==4) || (PTCODE==5)) {
      PTRACE(4, "H263+", "Header\tELNUM: " << headerBits.GetBits(4));
      if (UFEP==1)
        PTRACE(4, "H263+", "Header\tRLNUM: " << headerBits.GetBits(4));
    }

    if (RPS) {
        PTRACE(1, "H263+", "Header\tDecoding of RPS parameters not supported");
        return 0;
    }  
    PTRACE(4, "H263+", "Header\tPQUANT: " << headerBits.GetBits(5));                    // PQUANT
  } else {
    PTRACE(4, "H263+", "Header\tPicture: " << formats[PTYPEFormat] 
                                   << ", " << (headerBits.GetBit() ? "P-Picture" : "I-Picture")  // still PTYPE
                               << " UMV: " << headerBits.GetBit() 
                               << " SAC: " << headerBits.GetBit() 
                               << " APC: " << headerBits.GetBit());
    PB = headerBits.GetBit();                                                      // PB
    PTRACE(4, "H263+", "Header\tPB-Frames: " << PB);
    PTRACE(4, "H263+", "Header\tPQUANT: " << headerBits.GetBits(5));                    // PQUANT

    CPM = headerBits.GetBit();
    if (CPM) {
      PTRACE(4, "H263+", "Header\tCPM: " << CPM << " PSBI: " << headerBits.GetBits(2)); // CPM + PSBI
    } else {
      PTRACE(4, "H263+", "Header\tCPM: " << CPM ); 
    }
  }	

  if (PB)
    PTRACE(4, "H263+", "Header\tTRB: " << headerBits.GetBits (3)                     // TRB
                    << " DBQUANT: " << headerBits.GetBits (2));                   // DQUANT

  while (headerBits.GetBits (1)) {                                                // PEI bit
    PTRACE(4, "H263+", "Header\tPSUPP: " << headerBits.GetBits (8));                 // PSPARE bits 
  }

  return headerBits.GetPos();
} 
