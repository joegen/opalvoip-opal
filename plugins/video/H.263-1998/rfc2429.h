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


class PluginCodec_RTP;


typedef struct data_t
{
  uint8_t* ptr;
  size_t pos;
  size_t len;
} data_t;

typedef struct header_data_t
{
  uint8_t  buf[255];
  size_t len;
  unsigned pebits;
} header_data_t;

class Bitstream
{
public:
  Bitstream ();
  void SetBytes (uint8_t* data, size_t dataLen, uint8_t sbits, uint8_t ebits);
  void GetBytes (uint8_t** data, size_t * dataLen);
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
  virtual bool GetPacket(PluginCodec_RTP & frame, unsigned int & flags);
  virtual BYTE * GetBuffer();
  virtual size_t GetMaxSize();
  virtual bool SetLength(size_t len);

  virtual void NewFrame();
  virtual bool AddPacket(const PluginCodec_RTP & frame);
  virtual bool IsValid();
  virtual bool IsIntraFrame();
  virtual size_t GetLength() { return m_encodedFrame.len; }

private:
  size_t parseHeader(uint8_t* headerPtr, size_t headerMaxLen);

  size_t m_minPayloadSize;
  size_t m_maxFrameSize;
  bool     m_customClock;
  data_t   m_encodedFrame;
  header_data_t m_picHeader;
  std::vector<size_t> m_startCodes;
};

#endif /* __H263PFrame_H__ */
