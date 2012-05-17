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


#ifndef __H264FRAME_H__
#define __H264FRAME_H__ 1

// GVX 3000 does not like STAP packets... So we waste 40 bytes per connection...
//#define SEND_STAP_PACKETS 1

#include "../../common/ffmpeg.h"
#include <codec/opalplugin.hpp>

#include <vector>


#define H264_NAL_TYPE_NON_IDR_SLICE 1
#define H264_NAL_TYPE_DP_A_SLICE 2
#define H264_NAL_TYPE_DP_B_SLICE 3
#define H264_NAL_TYPE_DP_C_SLICE 0x4
#define H264_NAL_TYPE_IDR_SLICE 0x5
#define H264_NAL_TYPE_SEI 0x6
#define H264_NAL_TYPE_SEQ_PARAM 0x7
#define H264_NAL_TYPE_PIC_PARAM 0x8
#define H264_NAL_TYPE_ACCESS_UNIT 0x9
#define H264_NAL_TYPE_END_OF_SEQ 0xa
#define H264_NAL_TYPE_END_OF_STREAM 0xb
#define H264_NAL_TYPE_FILLER_DATA 0xc
#define H264_NAL_TYPE_SEQ_EXTENSION 0xd



class H264Frame : public FFMPEGCodec::EncodedFrame
{
public:
  H264Frame();

  virtual const char * GetName() const { return "RFC3984"; }
  virtual bool Reset(size_t len = 0);
  virtual bool GetPacket(PluginCodec_RTP & rtp, unsigned & flags);
  virtual bool AddPacket(const PluginCodec_RTP & frame, unsigned & flags);


  void Allocate(uint32_t numberOfNALs);
  void AddNALU(uint8_t type, uint32_t length, const uint8_t * payload);

  void SetTimestamp (uint32_t timestamp) 
  {
    m_timestamp = timestamp;
  }
  bool HasRTPFrames ()
  {
    return m_currentNAL < m_numberOfNALsInFrame;
  }

  bool IsSync ();
  
private:
  bool EncapsulateSTAP  (PluginCodec_RTP & frame, unsigned int & flags);
  bool EncapsulateFU    (PluginCodec_RTP & frame, unsigned int & flags);

  bool DeencapsulateFU   (const PluginCodec_RTP & frame);
  bool DeencapsulateSTAP (const PluginCodec_RTP & frame);
  bool AddDataToEncodedFrame (uint8_t *data, uint32_t dataLen, uint8_t header, bool addHeader);
  bool IsStartCode (const uint8_t *positionInFrame);

    // general stuff
  uint32_t m_timestamp;

  struct NALU
  {
    uint8_t  type;
    uint32_t offset;
    uint32_t length;
  };

  std::vector<NALU> m_NALs;
  uint32_t m_numberOfNALsInFrame;
  uint32_t m_currentNAL; 
  uint32_t m_numberOfNALsReserved;
  
  // for encapsulation
  uint32_t m_currentNALFURemainingLen;
  uint8_t* m_currentNALFURemainingDataPtr;
  uint8_t  m_currentNALFUHeader0;
  uint8_t  m_currentNALFUHeader1;

  // for deencapsulation
  uint16_t m_currentFU;
};

#endif /* __H264FRAME_H__ */
