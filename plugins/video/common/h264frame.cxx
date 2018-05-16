/*****************************************************************************/
/* The contents of this file are subject to the Mozilla Public License       */
/* Version 1.0 (the "License"); you may not use this file except in          */
/* compliance with the License.  You may obtain a copy of the License at     */
/* http://www.mozilla.org/MPL/                                               */
/*                                                                           */
/* Software distributed under the License is distributed on an "AS IS"       */
/* basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the  */
/* License for the specific language governing rights and limitations under  */
/* the License.                                                              */
/*                                                                           */
/* The Original Code is the Open H323 Library.                               */
/*                                                                           */
/* The Initial Developer of the Original Code is Matthias Schneider          */
/* Copyright (C) 2007 Matthias Schneider, All Rights Reserved.               */
/*                                                                           */
/* Contributor(s): Matthias Schneider (ma30002000@yahoo.de)                  */
/*                                                                           */
/* Alternatively, the contents of this file may be used under the terms of   */
/* the GNU General Public License Version 2 or later (the "GPL"), in which   */
/* case the provisions of the GPL are applicable instead of those above.  If */
/* you wish to allow use of your version of this file only under the terms   */
/* of the GPL and not to allow others to use your version of this file under */
/* the MPL, indicate your decision by deleting the provisions above and      */
/* replace them with the notice and other provisions required by the GPL.    */
/* If you do not delete the provisions above, a recipient may use your       */
/* version of this file under either the MPL or the GPL.                     */
/*                                                                           */
/* The Original Code was written by Matthias Schneider <ma30002000@yahoo.de> */
/*****************************************************************************/

#include "h264frame.h"


static uint8_t const StartCode[] = { 0, 0, 0, 1 };


H264Frame::H264Frame()
  : m_profile(0)
  , m_level(0)
  , m_constraint_set0(false)
  , m_constraint_set1(false)
  , m_constraint_set2(false)
  , m_constraint_set3(false)
  , m_packetisationMode(0)
  , m_timestamp(0)
  , m_receiverState(e_NormalProcessing)
{
  m_maxPayloadSize = 1400;
  Reset();
}


bool H264Frame::Reset(size_t len)
{
  m_numberOfNALsInFrame = 0;
  m_currentNAL = 0; 

  m_currentNALFURemainingLen = 0;
  m_currentNALFURemainingDataPtr = NULL;
  m_currentNALFUHeader0 = 0;
  m_currentNALFUHeader1 = 0;

  m_currentFU = 0;

  return OpalPluginFrame::Reset(len);
}


void H264Frame::Allocate(uint32_t numberOfNALs)
{
  m_NALs.resize(numberOfNALs);
}


static void SkipStartCode(const uint8_t * & payload, size_t & length)
{
  if (memcmp(payload, StartCode, sizeof(StartCode)) == 0) {
    payload += sizeof(StartCode);
    length -= sizeof(StartCode);
  }
  else if (memcmp(payload, StartCode+1, sizeof(StartCode)-1) == 0) {
    payload += sizeof(StartCode)-1;
    length -= sizeof(StartCode)-1;
  }
}


bool H264Frame::AddNALU(uint8_t type, size_t length, const uint8_t * payload)
{
  if (payload != NULL)
    SkipStartCode(payload, length);

  if (m_numberOfNALsInFrame + 1 >= m_NALs.size())
    m_NALs.resize(m_numberOfNALsInFrame + 1);

  m_NALs[m_numberOfNALsInFrame].type = type;
  m_NALs[m_numberOfNALsInFrame].length = (uint32_t)length;
  m_NALs[m_numberOfNALsInFrame].offset = (uint32_t)m_length;
  ++m_numberOfNALsInFrame;

  if (payload != NULL) {
    if (!Append(payload, length))
      return false;

    if (type == H264_NAL_TYPE_SEQ_PARAM)
      SetSPS(payload+1);
  }

  return true;
}


bool H264Frame::GetPacket(PluginCodec_RTP & frame, unsigned int & flags)
{
  if (m_currentNAL >= m_numberOfNALsInFrame) 
    return false;

  uint32_t curNALLen = m_NALs[m_currentNAL].length;
  const uint8_t *curNALPtr = m_buffer + m_NALs[m_currentNAL].offset;

  /*
    * We have 3 types of packets we can send:
    * fragmentation units - if the NAL is > max_payload_size
    * single nal units - if the NAL is < max_payload_size, and can only fit 1 NAL
    * single time aggregation units - if we can put multiple NALs into one packet
    *
    * We don't send multiple time aggregation units
    */

  if (m_packetisationMode > 0) {
    // fragmentation unit - break up into max_payload_size size chunks
    if (curNALLen > m_maxPayloadSize)
      return EncapsulateFU(frame, flags);

    // it is the last NAL of that frame or doesnt fit into an STAP packet with next nal ?
    if (((m_currentNAL + 1) < m_numberOfNALsInFrame) &&
      ((curNALLen + m_NALs[m_currentNAL + 1].length + 5) <= m_maxPayloadSize))
      return EncapsulateSTAP(frame, flags);
  }
  else {
    if (curNALLen > m_maxPayloadSize) {
      PTRACE(2, GetName(), "Cannot use mode zero for " << curNALLen << " byte NALU with " << m_maxPayloadSize << " byte packet");
      return false;
    }
  }

  // single nal unit packet
  frame.SetPayloadSize(curNALLen);
  memcpy(frame.GetPayloadPtr(), curNALPtr, curNALLen);
  frame.SetTimestamp(m_timestamp);
  frame.SetMarker((m_currentNAL + 1) >= m_numberOfNALsInFrame ? 1 : 0);
  if (frame.GetMarker())
    flags |= PluginCodec_ReturnCoderLastFrame;  // marker bit on last frame of video

  PTRACE(6, GetName(), "Encapsulating NAL unit #" << m_currentNAL << "/" << (m_numberOfNALsInFrame-1) << " of " << curNALLen << " bytes as a regular NAL unit");
  m_currentNAL++;
  return true;
}

bool H264Frame::EncapsulateSTAP(PluginCodec_RTP & frame, unsigned int & flags)
{
  uint32_t STAPLen = 1;
  uint32_t highestNALNumberInSTAP = m_currentNAL;
  
  // first check how many nals we want to put into the packet
  do {
    STAPLen += 2;
    STAPLen +=  m_NALs[highestNALNumberInSTAP].length;
    highestNALNumberInSTAP++;
  } while (highestNALNumberInSTAP < m_numberOfNALsInFrame && STAPLen < m_maxPayloadSize);

  if (STAPLen > m_maxPayloadSize)
  {
    STAPLen -= 2;
    STAPLen -= m_NALs[(highestNALNumberInSTAP-1)].length;
    highestNALNumberInSTAP--;
  }

  PTRACE(6, GetName(), "Encapsulating NAL units " << m_currentNAL << "-"<< (highestNALNumberInSTAP-1) << "/" << (m_numberOfNALsInFrame-1) << " as a STAP of " << STAPLen);

  frame.SetPayloadSize(1); // for stap header

  uint32_t curNALLen;
  const uint8_t* curNALPtr;
  uint8_t  maxNRI = 0;
  while (m_currentNAL < highestNALNumberInSTAP) {
    curNALLen = m_NALs[m_currentNAL].length;
    curNALPtr = m_buffer + m_NALs[m_currentNAL].offset;

    // store the nal length information
    frame.SetPayloadSize(frame.GetPayloadSize() + 2);
    *((uint8_t*)frame.GetPayloadPtr() + frame.GetPayloadSize() - 2) = (uint8_t)(curNALLen >> 8);
    *((uint8_t*)frame.GetPayloadPtr() + frame.GetPayloadSize() - 1) = (uint8_t) curNALLen;

    // store the nal
    frame.SetPayloadSize(frame.GetPayloadSize() + curNALLen);
    memcpy ((uint8_t*)frame.GetPayloadPtr() + frame.GetPayloadSize() - curNALLen, (void *)curNALPtr, curNALLen);

    if ((*curNALPtr & 0x60) > maxNRI) maxNRI = *curNALPtr & 0x60;
    PTRACE(6, GetName(), "Adding NAL unit " << m_currentNAL << "/" << (m_numberOfNALsInFrame-1) << " of " << curNALLen << " bytes to STAP");
    m_currentNAL++;
  }

  // set the nri value in the stap header
*frame.GetPayloadPtr() = (unsigned char)(H264_NAL_TYPE_STAP | maxNRI);
  frame.SetTimestamp(m_timestamp);
  frame.SetMarker(m_currentNAL >= m_numberOfNALsInFrame ? 1 : 0);
  if (frame.GetMarker())
    flags |= PluginCodec_ReturnCoderLastFrame;  // marker bit on last frame of video

  return true;
}


bool H264Frame::EncapsulateFU(PluginCodec_RTP & frame, unsigned int & flags)
{
  uint8_t header[2];
  uint32_t curFULen;

  if ((m_currentNALFURemainingLen==0) || (m_currentNALFURemainingDataPtr==NULL))
  {
    m_currentNALFURemainingLen = m_NALs[m_currentNAL].length;
    m_currentNALFURemainingDataPtr = m_buffer + m_NALs[m_currentNAL].offset;
    m_currentNALFUHeader0 = (*m_currentNALFURemainingDataPtr & 0x60) | H264_NAL_TYPE_FU;
    m_currentNALFUHeader1 = *m_currentNALFURemainingDataPtr & 0x1f;
    header[0] = m_currentNALFUHeader0;
    header[1] = 0x80 | m_currentNALFUHeader1; // s indication
    m_currentNALFURemainingDataPtr++; // remove the first byte
    m_currentNALFURemainingLen--;
  }
  else
  {
    header[0] = m_currentNALFUHeader0;
    header[1] = m_currentNALFUHeader1;
  }

  if (m_currentNALFURemainingLen > 0)
  {
    bool last = false;
    if ((m_currentNALFURemainingLen + 2) <= m_maxPayloadSize)
    {
      header[1] |= 0x40;
      curFULen = m_currentNALFURemainingLen;
      last = true;
    }
    else
    {
      curFULen = (uint32_t)(m_maxPayloadSize - 2);
    }

    frame.SetPayloadSize(curFULen + 2);
    memcpy ((uint8_t*)frame.GetPayloadPtr(), header, 2);
    memcpy ((uint8_t*)frame.GetPayloadPtr()+2, m_currentNALFURemainingDataPtr, curFULen);
    frame.SetTimestamp(m_timestamp);
    frame.SetMarker((last && ((m_currentNAL+1) >= m_numberOfNALsInFrame)) ? 1 : 0);
    if (frame.GetMarker())
      flags |= PluginCodec_ReturnCoderLastFrame;  // marker bit on last frame of video

    m_currentNALFURemainingDataPtr += curFULen;
    m_currentNALFURemainingLen -= curFULen;
    PTRACE(6, GetName(), "Encapsulating "<< curFULen << " bytes of NAL " << m_currentNAL<< "/" << (m_numberOfNALsInFrame-1) << " as a FU (" << m_currentNALFURemainingLen << " bytes remaining)");
  } 
  if (m_currentNALFURemainingLen==0)
  {
    m_currentNAL++;
    m_currentNALFURemainingDataPtr=NULL;
  }
  return true;
}

bool H264Frame::AddPacket(const PluginCodec_RTP & frame, unsigned & flags)
{
  if (flags&PluginCodec_CoderPacketLoss) {
    flags = PluginCodec_ReturnCoderRequestIFrame;
    m_receiverState = e_AwaitingMarker;
    Reset();
    return true;
  }

  const uint8_t * payloadPtr = frame.GetPayloadPtr();
  size_t payloadSize = frame.GetPayloadSize();

  // Some clients stupidly send the start code in the RTP packet
  SkipStartCode(payloadPtr, payloadSize);

  uint8_t curNALType = *payloadPtr & 0x1f;

  switch (m_receiverState) {
    case e_AwaitingMarker :
      if (frame.GetMarker())
        m_receiverState = e_AwaitingIntra;
      return true;

    case e_AwaitingIntra :
      if (curNALType != H264_NAL_TYPE_SEQ_PARAM && (curNALType != H264_NAL_TYPE_STAP || payloadPtr[3] != H264_NAL_TYPE_SEQ_PARAM))
        return true;

      m_receiverState = e_NormalProcessing;
    default :
      break;
  }

  if (curNALType >= H264_NAL_TYPE_NON_IDR_SLICE && curNALType <= H264_NAL_TYPE_FILLER_DATA) {
    // regular NAL - put in buffer, adding the header.
    PTRACE(6, GetName(), "Deencapsulating a regular NAL unit NAL of " << payloadSize - 1 << " bytes (type " << (int) curNALType << ")");
    return AddDataToEncodedFrame(payloadPtr + 1, payloadSize - 1, *payloadPtr, true);
  }

  if (curNALType == H264_NAL_TYPE_STAP) 
  {
    // stap-A (single time aggregation packet )
    if (DeencapsulateSTAP(payloadPtr, payloadSize))
      return true;
  } 
  else if (curNALType == H264_NAL_TYPE_FU) 
  {
    // Fragmentation Units
    if (DeencapsulateFU(payloadPtr, payloadSize))
      return true;
  }
  else
  {
    PTRACE(2, GetName(), "Skipping unsupported NAL unit type " << (unsigned)curNALType);
  }

  Reset();
  flags |= PluginCodec_ReturnCoderRequestIFrame;
  return true;
}


bool H264Frame::IsIntraFrame() const
{
  uint32_t i;

  for (i=0; i < m_numberOfNALsInFrame; i++)
  {
    if ((m_NALs[i].type == H264_NAL_TYPE_IDR_SLICE) ||
        (m_NALs[i].type == H264_NAL_TYPE_SEQ_PARAM) ||
        (m_NALs[i].type == H264_NAL_TYPE_PIC_PARAM))
    {
      return true;
    }
  }
  return false;
}


bool H264Frame::DeencapsulateSTAP(const uint8_t *payloadPtr, size_t payloadSize)
{
  const uint8_t* curSTAP = payloadPtr + 1;
  size_t curSTAPLen = payloadSize - 1; 

  PTRACE(6, GetName(), "Deencapsulating a STAP of " << curSTAPLen << " bytes");
  while (curSTAPLen > 0)
  {
    // first, theres a 2 byte length field
    uint32_t len = (curSTAP[0] << 8) | curSTAP[1];
    if (len > payloadSize)
      return false;
    curSTAP += 2;
    // then the header, followed by the body.  We'll add the header
    // in the AddDataToEncodedFrame - that's why the nal body is dptr + 1
    PTRACE(6, GetName(), "Deencapsulating an NAL unit of " << len << " bytes (type " << (int)(*curSTAP && 0x1f) << ") from STAP");
    if (!AddDataToEncodedFrame(curSTAP + 1,  len - 1, *curSTAP, true))
      return false;

    curSTAP += len;
    if ((len + 2) > curSTAPLen)
    {
      curSTAPLen = 0;
      PTRACE(2, GetName(), "Error deencapsulating STAP, STAP header says its " << len + 2 << " bytes long but there are only " << curSTAPLen << " bytes left of the packet");
      return false;
    }
    else
    {
      curSTAPLen -= (len + 2);
    }
  }
  return true;
}


bool H264Frame::DeencapsulateFU(const uint8_t *payloadPtr, size_t payloadSize)
{
  const uint8_t* curFUPtr = payloadPtr;
  size_t curFULen = payloadSize; 
  uint8_t header;

  if ((curFUPtr[1] & 0x80) && !(curFUPtr[1] & 0x40))
  {
    PTRACE(6, GetName(), "Deencapsulating a FU of " << payloadSize - 1 << " bytes (Startbit, !Endbit)");
    if (m_currentFU) {
      m_currentFU=1;
    }
    else
    {
      m_currentFU++;
      header = (curFUPtr[0] & 0xe0) | (curFUPtr[1] & 0x1f);
      if (!AddDataToEncodedFrame(curFUPtr + 2, curFULen - 2, header,  true))
        return false;
    }
  } 
  else if (!(curFUPtr[1] & 0x80) && !(curFUPtr[1] & 0x40))
  {
    PTRACE(6, GetName(), "Deencapsulating a FU of " << payloadSize - 1 << " bytes (!Startbit, !Endbit)");
    if (m_currentFU)
    {
      m_currentFU++;
      if (!AddDataToEncodedFrame(curFUPtr + 2, curFULen - 2,  0, false))
        return false;
    }
    else
    {
      m_currentFU=0;
      PTRACE(2, GetName(), "Received an intermediate FU without getting the first - dropping!");
      return false;
    }
  } 
  else if (!(curFUPtr[1] & 0x80) && (curFUPtr[1] & 0x40))
  {
    PTRACE(6, GetName(), "Deencapsulating a FU of " << payloadSize - 1 << " bytes (!Startbit, Endbit)");
    if (m_currentFU) {
      m_currentFU=0;
      if (!AddDataToEncodedFrame( curFUPtr + 2, curFULen - 2, 0, false))
        return false;
    }
    else
    {
      m_currentFU=0;
      PTRACE(2, GetName(), "Received a last FU without getting the first - dropping!");
      return false;
    }
  } 
  else if ((curFUPtr[1] & 0x80) && (curFUPtr[1] & 0x40))
  {
    PTRACE(6, GetName(), "Deencapsulating a FU of " << payloadSize - 1 << " bytes (Startbit, Endbit)");
    PTRACE(2, GetName(), "Received a FU with both Starbit and Endbit set - This MUST NOT happen!");
    m_currentFU=0;
    return false;
  } 
  return true;
}


void H264Frame::SetSPS(const uint8_t * payload)
{
  m_profile = payload[0];
  m_constraint_set0 = (payload[1] & 0x80) != 0;
  m_constraint_set1 = (payload[1] & 0x40) != 0;
  m_constraint_set2 = (payload[1] & 0x20) != 0;
  m_constraint_set3 = (payload[1] & 0x10) != 0;
  m_level = payload[2];

  PTRACE(4, GetName(), "Profile: " << m_profile << 
                       " Level: "   << m_level << 
                       " Constraints: 0=" << m_constraint_set0 <<
                                    " 1=" << m_constraint_set1 <<
                                    " 2=" << m_constraint_set2 <<
                                    " 3=" << m_constraint_set3);
}


bool H264Frame::AddDataToEncodedFrame(const uint8_t *data, size_t payloadSize, uint8_t header, bool addHeader)
{
  if (addHeader) 
  {
    PTRACE(6, GetName(), "Adding a NAL unit of " << payloadSize << " bytes to buffer (type " << (int)(header & 0x1f) << ")"); 
    if (((header & 0x1f) == H264_NAL_TYPE_SEQ_PARAM) && (payloadSize >= 3))
      SetSPS(data);

    // add 00 00 01 [headerbyte] header
    if (!Append(StartCode, sizeof(StartCode)) ||
        !AddNALU(header & 0x1f, payloadSize + 1, NULL) ||
        !Append(&header, 1))
      return false;
  }
  else {
    PTRACE(6, GetName(), "Adding a NAL unit of " << payloadSize << " bytes to buffer");
    m_NALs[m_numberOfNALsInFrame - 1].length += (uint32_t)payloadSize;
  }

  PTRACE(6, GetName(), "Reserved memory for  " << m_NALs.size() <<" NALs, Inframe/current: "<< m_numberOfNALsInFrame <<" Offset: "
    << m_NALs[m_numberOfNALsInFrame-1].offset << " Length: "<< m_NALs[m_numberOfNALsInFrame-1].length << " Type: "<< (int)(m_NALs[m_numberOfNALsInFrame-1].type));

  return Append(data, payloadSize);
}
