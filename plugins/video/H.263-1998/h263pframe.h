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

#ifndef __H263PFrame_H__
#define __H263PFrame_H__ 1

#include "../common/ffmpeg.h"
#include "../common/rtpframe.h"


extern "C" {
#include LIBAVCODEC_HEADER
};

typedef struct data_t
{
  uint8_t* ptr;
  uint32_t pos;
  uint32_t len;
} data_t;

typedef struct header_data_t
{
  uint8_t  buf[255];
  uint32_t len;
  uint32_t pebits;
} header_data_t;

class Bitstream
{
public:
  Bitstream ();
  void SetBytes (uint8_t* data, uint32_t dataLen, uint8_t sbits, uint8_t ebits);
  void GetBytes (uint8_t** data, uint32_t * dataLen);
  uint32_t GetBits (uint32_t numBits);
  uint32_t PeekBits (uint32_t numBits);
  void PutBits(uint32_t posBits, uint32_t numBits, uint32_t value);
  void SetPos(uint32_t pos);
  uint32_t GetPos();
private:
  data_t  m_data;
  uint8_t m_sbits;
  uint8_t m_ebits;
};

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
class H263PFrame : public Packetizer
{
public:
  H263PFrame();
  ~H263PFrame();

  bool Reset(unsigned width, unsigned height);
  bool GetPacket(RTPFrame & frame, unsigned int & flags);
  unsigned char * GetBuffer();
  size_t GetMaxSize();
  bool SetLength(size_t len);

  bool SetFromRTPFrame (RTPFrame & frame, unsigned int & flags);

  bool HasRTPFrames ()
  {
    return (m_encodedFrame.pos < m_encodedFrame.len);
  }

  uint32_t GetLength()
  {
    return (m_encodedFrame.len);
  }

  void SetMaxPayloadSize (uint16_t maxPayloadSize) 
  {
    m_maxPayloadSize = maxPayloadSize;
  }

  
  bool hasPicHeader ();
  bool IsIFrame ();
private:
  uint32_t parseHeader(uint8_t* headerPtr, uint32_t headerMaxLen);

  uint16_t m_maxPayloadSize;
  uint16_t m_minPayloadSize;
  uint32_t m_maxFrameSize;
  bool     m_customClock;
  data_t   m_encodedFrame;
  header_data_t m_picHeader;
  std::vector<uint32_t> m_startCodes;
};

#endif /* __H263PFrame_H__ */
