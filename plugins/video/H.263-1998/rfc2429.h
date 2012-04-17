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

#include "h263-1998.h"

#include <vector>


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

class RFC2429Frame : public Packetizer, public Depacketizer
{
public:
  RFC2429Frame();
  ~RFC2429Frame();

  virtual bool Reset(unsigned width, unsigned height);
  virtual bool GetPacket(RTPFrame & frame, unsigned int & flags);
  virtual BYTE * GetBuffer();
  virtual size_t GetMaxSize();
  virtual bool SetLength(size_t len);

  virtual void NewFrame();
  virtual bool AddPacket(const RTPFrame & frame);
  virtual bool IsValid();
  virtual bool IsIntraFrame();
  virtual size_t GetLength() { return m_encodedFrame.len; }

private:
  uint32_t parseHeader(uint8_t* headerPtr, uint32_t headerMaxLen);

  uint16_t m_minPayloadSize;
  uint32_t m_maxFrameSize;
  bool     m_customClock;
  data_t   m_encodedFrame;
  header_data_t m_picHeader;
  std::vector<uint32_t> m_startCodes;
};

#endif /* __H263PFrame_H__ */
