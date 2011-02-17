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

#ifndef _MSC_VER
#include "plugin-config.h"
#endif

#include "h264frame.h"
#include "trace.h"

#include <stdlib.h>
#include <string.h>

#ifndef LICENCE_MPL
  #ifndef _WIN32
    #include "../gpl/enc-ctx.h"
    #include "../gpl/x264loader_unix.h"
    extern X264Library X264Lib;
  #else
    #define X264_NAL_ENCODE x264_nal_encode 
  #endif
#endif
#define MAX_FRAME_SIZE 128 * 1024

H264Frame::H264Frame ()
{
  m_timestamp = 0;
  m_maxPayloadSize = 1400;
  m_encodedFrame = (uint8_t*)malloc(MAX_FRAME_SIZE);
  m_NALs = NULL;
  m_numberOfNALsReserved = 0;

  BeginNewFrame();
}

void H264Frame::BeginNewFrame ()
{
  m_encodedFrameLen = 0;

  m_numberOfNALsInFrame = 0;
  m_currentNAL = 0; 

  m_currentNALFURemainingLen = 0;
  m_currentNALFURemainingDataPtr = NULL;
  m_currentNALFUHeader0 = 0;
  m_currentNALFUHeader1 = 0;

  m_currentFU = 0;

}

H264Frame::~H264Frame ()
{
  if (m_encodedFrame)
    free(m_encodedFrame);
  if (m_NALs)
    free(m_NALs);
}

#ifndef LICENCE_MPL
void H264Frame::SetFromFrame (x264_nal_t *NALs, int numberOfNALs) {
  int vopBufferLen;
  int currentNAL = 0;

  uint8_t* currentPositionInFrame=(uint8_t*) m_encodedFrame;
  if (m_NALs) free(m_NALs);
  m_NALs = (h264_nal_t *)malloc(numberOfNALs * sizeof(h264_nal_t));

  m_encodedFrameLen = 0;
  m_numberOfNALsInFrame = 0;
  m_currentNAL=0;

  // read the nals out of the encoder and create meta data about their location and size in the frame
  for (currentNAL = 0; currentNAL < numberOfNALs; currentNAL++) {
    int currentNALLen;
    currentNALLen = X264_NAL_ENCODE(currentPositionInFrame, &vopBufferLen, &NALs[currentNAL]);
    if (currentNALLen > 0) 
    {
      m_NALs[m_numberOfNALsInFrame].length = currentNALLen;
      m_NALs[m_numberOfNALsInFrame].offset = m_encodedFrameLen;
      m_NALs[m_numberOfNALsInFrame].type = NALs[currentNAL].i_type;
      uint32_t header = 0;
      if (IsStartCode(currentPositionInFrame))
      {
	header = currentPositionInFrame[2] == 1 ? 3 : 4;
      }
      m_NALs[m_numberOfNALsInFrame].length -= header;
      m_NALs[m_numberOfNALsInFrame].offset += header;
      TRACE_UP(4, "H264\tEncap\tLoaded NAL unit #" << currentNAL << " - type " << NALs[currentNAL].i_type);

      uint8_t* NALptr = NALs[currentNAL].p_payload;
      if ( Trace::CanTraceUserPlane(4) && (NALs[currentNAL].i_type == H264_NAL_TYPE_SEQ_PARAM)) 
      {
      TRACE_UP(4,   "H264\tEncap\tProfile: " << (int)NALptr[0] << 
                                " Level: "   << (int)NALptr[2] << 
		   	        " Constraints: " << (NALptr[1] & 0x80 ? 1 : 0) 
			                         << (NALptr[1] & 0x40 ? 1 : 0) 
					         << (NALptr[1] & 0x20 ? 1 : 0) 
					         << (NALptr[1] & 0x10 ? 1 : 0));
      }

      m_numberOfNALsInFrame++;
      m_encodedFrameLen += currentNALLen;
      currentPositionInFrame += currentNALLen;
    } 
    else
    {
      TRACE_UP(4,"[enc] Need to increase vop buffer size by  " << -currentNALLen);
    }
  }
  TRACE_UP(4, "H264\tEncap\tLoaded an encoded frame of " << m_encodedFrameLen << " bytes consisiting of " << numberOfNALs << " NAL units");
}
#endif

bool H264Frame::GetRTPFrame(RTPFrame & frame, unsigned int & flags)
{
  flags = 0;
  flags |= (IsSync()) ? isIFrame : 0;
  if (m_currentNAL < m_numberOfNALsInFrame) 
  { 
    uint32_t curNALLen = m_NALs[m_currentNAL].length;
    const uint8_t *curNALPtr = m_encodedFrame + m_NALs[m_currentNAL].offset;
    /*
     * We have 3 types of packets we can send:
     * fragmentation units - if the NAL is > max_payload_size
     * single nal units - if the NAL is < max_payload_size, and can only fit 1 NAL
     * single time aggregation units - if we can put multiple NALs into one packet
     *
     * We don't send multiple time aggregation units
     */

    if (curNALLen > m_maxPayloadSize)
    {
      // fragmentation unit - break up into max_payload_size size chunks
      return EncapsulateFU(frame, flags);
    } 
    else 
    {
      // it is the last NAL of that frame or doesnt fit into an STAP packet with next nal ?
#ifdef SEND_STAP_PACKETS
      if (((m_currentNAL + 1) >= m_numberOfNALsInFrame)  ||  
	  ((curNALLen + m_NALs[m_currentNAL + 1].length + 5) > m_maxPayloadSize)) 
      { 
#endif
        // single nal unit packet

        frame.SetPayloadSize(curNALLen);
        memcpy(frame.GetPayloadPtr(), curNALPtr, curNALLen);
        frame.SetTimestamp(m_timestamp);
        frame.SetMarker((m_currentNAL + 1) >= m_numberOfNALsInFrame ? 1 : 0);
        flags |= frame.GetMarker() ? isLastFrame : 0;  // marker bit on last frame of video

        TRACE_UP(4, "H264\tEncap\tEncapsulating NAL unit #" << m_currentNAL << "/" << (m_numberOfNALsInFrame-1) << " of " << curNALLen << " bytes as a regular NAL unit");
        m_currentNAL++;
        return true;
#ifdef SEND_STAP_PACKETS
      } 
      else
      {
        return EncapsulateSTAP(frame, flags); 
      }
#endif
    }
  } 
  else 
  {
    return false;
  }
}

bool H264Frame::EncapsulateSTAP (RTPFrame & frame, unsigned int & flags) {
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

  TRACE_UP(4, "H264\tEncap\tEncapsulating NAL units " << m_currentNAL << "-"<< (highestNALNumberInSTAP-1) << "/" << (m_numberOfNALsInFrame-1) << " as a STAP of " << STAPLen);

  frame.SetPayloadSize(1); // for stap header

  uint32_t curNALLen;
  const uint8_t* curNALPtr;
  uint8_t  maxNRI = 0;
  while (m_currentNAL < highestNALNumberInSTAP) {
    curNALLen = m_NALs[m_currentNAL].length;
    curNALPtr = m_encodedFrame + m_NALs[m_currentNAL].offset;

    // store the nal length information
    frame.SetPayloadSize(frame.GetPayloadSize() + 2);
    *((uint8_t*)frame.GetPayloadPtr() + frame.GetPayloadSize() - 2) = (uint8_t)(curNALLen >> 8);
    *((uint8_t*)frame.GetPayloadPtr() + frame.GetPayloadSize() - 1) = (uint8_t) curNALLen;

    // store the nal
    frame.SetPayloadSize(frame.GetPayloadSize() + curNALLen);
    memcpy ((uint8_t*)frame.GetPayloadPtr() + frame.GetPayloadSize() - curNALLen, (void *)curNALPtr, curNALLen);

    if ((*curNALPtr & 0x60) > maxNRI) maxNRI = *curNALPtr & 0x60;
    TRACE_UP(4, "H264\tEncap\tAdding NAL unit " << m_currentNAL << "/" << (m_numberOfNALsInFrame-1) << " of " << curNALLen << " bytes to STAP");
    m_currentNAL++;
  }

  // set the nri value in the stap header
  //uint8_t stap = 24 | maxNRI;
  //memcpy (frame.GetPayloadPtr(),&stap,1);
  memset (frame.GetPayloadPtr(), 24 | maxNRI, 1);
  frame.SetTimestamp(m_timestamp);
  frame.SetMarker(m_currentNAL >= m_numberOfNALsInFrame ? 1 : 0);
  flags |= frame.GetMarker() ? isLastFrame : 0;  // marker bit on last frame of video

  return true;
}


bool H264Frame::EncapsulateFU(RTPFrame & frame, unsigned int & flags) {
  uint8_t header[2];
  uint32_t curFULen;

  if ((m_currentNALFURemainingLen==0) || (m_currentNALFURemainingDataPtr==NULL))
  {
    m_currentNALFURemainingLen = m_NALs[m_currentNAL].length;
    m_currentNALFURemainingDataPtr = m_encodedFrame + m_NALs[m_currentNAL].offset;
    m_currentNALFUHeader0 = (*m_currentNALFURemainingDataPtr & 0x60) | 28;
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
      curFULen = m_maxPayloadSize - 2;
    }

    frame.SetPayloadSize(curFULen + 2);
    memcpy ((uint8_t*)frame.GetPayloadPtr(), header, 2);
    memcpy ((uint8_t*)frame.GetPayloadPtr()+2, m_currentNALFURemainingDataPtr, curFULen);
    frame.SetTimestamp(m_timestamp);
    frame.SetMarker((last && ((m_currentNAL+1) >= m_numberOfNALsInFrame)) ? 1 : 0);
    flags |= frame.GetMarker() ? isLastFrame : 0;  // marker bit on last frame of video

    m_currentNALFURemainingDataPtr += curFULen;
    m_currentNALFURemainingLen -= curFULen;
    TRACE_UP(4, "H264\tEncap\tEncapsulating "<< curFULen << " bytes of NAL " << m_currentNAL<< "/" << (m_numberOfNALsInFrame-1) << " as a FU (" << m_currentNALFURemainingLen << " bytes remaining)");
  } 
  if (m_currentNALFURemainingLen==0)
  {
    m_currentNAL++;
    m_currentNALFURemainingDataPtr=NULL;
  }
  return true;
}

bool H264Frame::SetFromRTPFrame(RTPFrame & frame, unsigned int & flags) {
  uint8_t curNALType = *(frame.GetPayloadPtr()) & 0x1f;

  if (curNALType >= H264_NAL_TYPE_NON_IDR_SLICE &&
      curNALType <= H264_NAL_TYPE_FILLER_DATA)
  {
    // regular NAL - put in buffer, adding the header.
    TRACE_UP(4, "H264\tDeencap\tDeencapsulating a regular NAL unit NAL of " << frame.GetPayloadSize() - 1 << " bytes (type " << (int) curNALType << ")");
    AddDataToEncodedFrame(frame.GetPayloadPtr() + 1, frame.GetPayloadSize() - 1, *(frame.GetPayloadPtr()), 1);
  } 
  else if (curNALType == 24) 
  {
    // stap-A (single time aggregation packet )
    return DeencapsulateSTAP (frame, flags);
  } 
  else if (curNALType == 28) 
  {
    // Fragmentation Units
    return DeencapsulateFU (frame, flags);
  }
  else
  {
    TRACE_UP(4, "H264\tDeencap\tSkipping unsupported NAL unit type " << curNALType);
    return false;
  }
  return true;
}
bool H264Frame::IsSync () {
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

bool H264Frame::DeencapsulateSTAP (RTPFrame & frame, unsigned int & /*flags*/) {
  uint8_t* curSTAP = frame.GetPayloadPtr() + 1;
  uint32_t curSTAPLen = frame.GetPayloadSize() - 1; 

  TRACE_UP(4, "H264\tDeencap\tDeencapsulating a STAP of " << curSTAPLen << " bytes");
  while (curSTAPLen > 0)
  {
    // first, theres a 2 byte length field
    uint32_t len = (curSTAP[0] << 8) | curSTAP[1];
    curSTAP += 2;
    // then the header, followed by the body.  We'll add the header
    // in the AddDataToEncodedFrame - that's why the nal body is dptr + 1
    TRACE_UP(4, "H264\tDeencap\tDeencapsulating an NAL unit of " << len << " bytes (type " << (int)(*curSTAP && 0x1f) << ") from STAP");
    AddDataToEncodedFrame(curSTAP + 1,  len - 1, *curSTAP, 1);
    curSTAP += len;
    if ((len + 2) > curSTAPLen)
    {
      curSTAPLen = 0;
      TRACE(1, "H264\tDeencap\tError deencapsulating STAP, STAP header says its " << len + 2 << " bytes long but there are only " << curSTAPLen << " bytes left of the packet");
      return false;
    }
    else
    {
      curSTAPLen -= (len + 2);
    }
  }
  return true;
}

bool H264Frame::DeencapsulateFU (RTPFrame & frame, unsigned int & /*flags*/) {
  uint8_t* curFUPtr = frame.GetPayloadPtr();
  uint32_t curFULen = frame.GetPayloadSize(); 
  uint8_t header;

  if ((curFUPtr[1] & 0x80) && !(curFUPtr[1] & 0x40))
  {
    TRACE_UP(4, "H264\tDeencap\tDeencapsulating a FU of " << frame.GetPayloadSize() - 1 << " bytes (Startbit, !Endbit)");
    if (m_currentFU) {
      m_currentFU=1;
    }
    else
    {
      m_currentFU++;
      header = (curFUPtr[0] & 0xe0) | (curFUPtr[1] & 0x1f);
      AddDataToEncodedFrame(curFUPtr + 2, curFULen - 2, header,  1);
    }
  } 
  else if (!(curFUPtr[1] & 0x80) && !(curFUPtr[1] & 0x40))
  {
    TRACE_UP(4, "H264\tDeencap\tDeencapsulating a FU of " << frame.GetPayloadSize() - 1 << " bytes (!Startbit, !Endbit)");
    if (m_currentFU)
    {
      m_currentFU++;
      AddDataToEncodedFrame(curFUPtr + 2, curFULen - 2,  0, 0);
    }
    else
    {
      m_currentFU=0;
      TRACE(1, "H264\tDeencap\tReceived an intermediate FU without getting the first - dropping!");
      return false;
    }
  } 
  else if (!(curFUPtr[1] & 0x80) && (curFUPtr[1] & 0x40))
  {
    TRACE_UP(4, "H264\tDeencap\tDeencapsulating a FU of " << frame.GetPayloadSize() - 1 << " bytes (!Startbit, Endbit)");
    if (m_currentFU) {
      m_currentFU=0;
      AddDataToEncodedFrame( curFUPtr + 2, curFULen - 2, 0, 0);
    }
    else
    {
      m_currentFU=0;
      TRACE(1, "H264\tDeencap\tReceived a last FU without getting the first - dropping!");
      return false;
    }
  } 
  else if ((curFUPtr[1] & 0x80) && (curFUPtr[1] & 0x40))
  {
    TRACE_UP(4, "H264\tDeencap\tDeencapsulating a FU of " << frame.GetPayloadSize() - 1 << " bytes (Startbit, Endbit)");
    TRACE(1, "H264\tDeencap\tReceived a FU with both Starbit and Endbit set - This MUST NOT happen!");
    m_currentFU=0;
    return false;
  } 
  return true;
}

void H264Frame::AddDataToEncodedFrame (uint8_t *data, uint32_t dataLen, uint8_t header, bool addHeader) {
  uint8_t headerLen= addHeader ? 5 : 0;
  uint8_t* currentPositionInFrame = m_encodedFrame + m_encodedFrameLen;

  if (addHeader) 
  {
    TRACE_UP(4, "H264\tDeencap\tAdding a NAL unit of " << dataLen << " bytes to buffer (type " << (int)(header & 0x1f) << ")"); 
    uint8_t* NALptr = data;
    if ( Trace::CanTraceUserPlane(4) && ((header & 0x1f) == H264_NAL_TYPE_SEQ_PARAM) && (dataLen >= 3)) 
    {
      TRACE_UP(4, "H264\tDeencap\tProfile: " << (int)NALptr[0] << 
                                " Level: "   << (int)NALptr[2] << 
			        " Constraints: " << (NALptr[1] & 0x80 ? 1 : 0) 
			                         << (NALptr[1] & 0x40 ? 1 : 0) 
					         << (NALptr[1] & 0x20 ? 1 : 0) 
					         << (NALptr[1] & 0x10 ? 1 : 0));
    }
  }
  else TRACE_UP(4, "H264\tDeencap\tAdding a NAL unit of " << dataLen << " bytes to buffer");

  if (m_encodedFrameLen + dataLen + headerLen > MAX_FRAME_SIZE) {
    TRACE(1, "H264\tDeencap\tFrame too big (" << m_encodedFrameLen + dataLen + headerLen << ">" << MAX_FRAME_SIZE << ")");
    return;
  }

  // add 00 00 01 [headerbyte] header
  if (addHeader)
  {
    *currentPositionInFrame++ = 0;
    *currentPositionInFrame++ = 0;
    *currentPositionInFrame++ = 0;
    *currentPositionInFrame++ = 1;

    if (m_numberOfNALsInFrame + 1 >(m_numberOfNALsReserved))
    {
      m_NALs = (h264_nal_t *)realloc(m_NALs, (m_numberOfNALsReserved + 1) * sizeof(h264_nal_t));
      m_numberOfNALsReserved++;
    }
    if (m_NALs)
    {
      m_NALs[m_numberOfNALsInFrame].offset = m_encodedFrameLen + 4;
      m_NALs[m_numberOfNALsInFrame].length = dataLen + 1;
      m_NALs[m_numberOfNALsInFrame].type = header & 0x1f;


      m_numberOfNALsInFrame++;
    }

    *currentPositionInFrame++ = header;
  }
  else
  {
    if (m_NALs)
      m_NALs[m_numberOfNALsInFrame - 1].length += dataLen;
  }

  TRACE_UP (4, "H264\tDeencap\tReserved memory for  " << m_numberOfNALsReserved <<" NALs, Inframe/current: "<< m_numberOfNALsInFrame <<" Offset: "
    << m_NALs[m_numberOfNALsInFrame-1].offset << " Length: "<< m_NALs[m_numberOfNALsInFrame-1].length << " Type: "<< (int)(m_NALs[m_numberOfNALsInFrame-1].type));

  memcpy(currentPositionInFrame, data, dataLen);
  m_encodedFrameLen += dataLen + headerLen;
}

bool H264Frame::IsStartCode (const uint8_t *positionInFrame)
{
  if (positionInFrame[0] == 0 &&
      positionInFrame[1] == 0 &&
    ((positionInFrame[2] == 1) ||
    ((positionInFrame[2] == 0) && positionInFrame[3] == 1))) 
  {
    return true;
  }
  return false;
}
