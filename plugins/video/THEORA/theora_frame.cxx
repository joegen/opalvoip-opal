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

#include <codec/opalplugin.hpp>

#include "theora_frame.h"
#include <stdlib.h>
#include <string.h>

#define MAX_FRAME_SIZE 128 * 1024
#define MAX_CONFIG_SIZE 16 * 1024

theoraFrame::theoraFrame ()
{
  _encodedData.ptr=(uint8_t*)malloc(MAX_FRAME_SIZE);
  _encodedData.len=0;
  _encodedData.pos=0;

  _packedConfigData.ptr=(uint8_t*)malloc(MAX_CONFIG_SIZE);
  _packedConfigData.len=0;
  _packedConfigData.pos=0;

  _frameCount = 0;
  _timestamp = 0;
  _ident = 0xffffffff;
  _maxPayloadSize = 1400;
  _configSent = false;
  _headerReturned = false;
  _isIFrame = false;

  BeginNewFrame();
}

void theoraFrame::BeginNewFrame ()
{
  _encodedData.len=0;
  _encodedData.pos=0;
}

theoraFrame::~theoraFrame ()
{
  if (_encodedData.ptr) free (_encodedData.ptr);
  if (_packedConfigData.ptr) free (_packedConfigData.ptr);
}

void theoraFrame::SetFromHeaderConfig (ogg_packet* headerPacket) {
  if (headerPacket->bytes != THEORA_HEADER_PACKET_SIZE ) {
    PTRACE(1, "THEORA", "Encap\tGot Header Packet from encoder that has len " << headerPacket->bytes << " != " << THEORA_HEADER_PACKET_SIZE);
    return;
  }

  memcpy (_packedConfigData.ptr, headerPacket->packet, THEORA_HEADER_PACKET_SIZE);
  if (_packedConfigData.len == 0) _packedConfigData.len = THEORA_HEADER_PACKET_SIZE;
  _packedConfigData.pos = 0;
  _configSent = false;
}

void theoraFrame::SetFromTableConfig (ogg_packet* tablePacket) {
  PTRACE(4, "THEORA", "Encap\tGot table packet with len " << tablePacket->bytes);

  memcpy (_packedConfigData.ptr + THEORA_HEADER_PACKET_SIZE, tablePacket->packet, tablePacket->bytes);
  _packedConfigData.len = tablePacket->bytes + THEORA_HEADER_PACKET_SIZE;
  _packedConfigData.pos = 0;
  _configSent = false;
}


void theoraFrame::SetFromFrame (ogg_packet* framePacket) {
  PTRACE(4, "THEORA", "Encap\tGot encoded frame packet with len " << framePacket->bytes);

  memcpy (_encodedData.ptr, framePacket->packet, framePacket->bytes);
  _encodedData.len = framePacket->bytes;
  _encodedData.pos = 0;
  _frameCount++;
  if ((_frameCount % THEORA_SEND_CONFIG_INTERVAL) == 0) _configSent = false;
}

void theoraFrame::assembleRTPFrame(PluginCodec_RTP & frame, data_t & frameData, bool sendPackedConfig) {
  uint8_t* dataPtr = frame.GetPayloadPtr() ;
  uint16_t len = 0;
  dataPtr[0] = 0xde;
  dataPtr[1] = 0xde;
  dataPtr[2] = 0xde;

  frame.SetMarker(0);

  if (frameData.pos > 0) {
    if ((frameData.len - frameData.pos) <= (_maxPayloadSize - 6)) {
      // last fragmentation packet
      dataPtr[3] = sendPackedConfig ? 0xD0 : 0xC0;
      len = frameData.len - frameData.pos;
      if (sendPackedConfig)  
        _configSent = true;
       else
        frame.SetMarker(1);
      PTRACE(4, "THEORA", "Encap\tEncapsulated fragmentation last packet with length of " << len << " bytes");
    } 
    else {
      // continuation fragmentation packet
      dataPtr[3] = sendPackedConfig ? 0x90 : 0x80;
      len = (_maxPayloadSize - 6);
      PTRACE(4, "THEORA", "Encap\tEncapsulated fragmentation continuation packet with length of " << len << " bytes");
    }
  }
  else {
    if (frameData.len <= (_maxPayloadSize - 6)) {
      // single packet
      dataPtr[3] = sendPackedConfig ? 0x11 : 0x01;
      len = frameData.len;
      if (sendPackedConfig) 
        _configSent = true;
       else
        frame.SetMarker(1);
      PTRACE(4, "THEORA", "Encap\tEncapsulated single packet with length of " << len << " bytes");
    } 
    else {
      // start fragmentation packet
      dataPtr[3] = sendPackedConfig ? 0x50 : 0x40;
      len = (_maxPayloadSize - 6);
      PTRACE(4, "THEORA", "Encap\tEncapsulated fragmentation start packet with length of " << len << " bytes");
    }
  }
  dataPtr[4] = (len >> 8);
  dataPtr[5] = len & 0xFF;
  memcpy (dataPtr + 6, frameData.ptr + frameData.pos, len);
  frameData.pos += len;
  if (frameData.pos == frameData.len) 
    frameData.pos = 0; 
  if (frameData.pos > frameData.len) 
    PTRACE(1, "THEORA", "Encap\tPANIC: " << frameData.pos << "<" << frameData.len );
  frame.SetTimestamp(_timestamp);
  frame.SetPayloadSize(len + 6);
}

void theoraFrame::GetRTPFrame(PluginCodec_RTP & frame, unsigned int & flags)
{
  flags = 0;
  flags |= IsIFrame() ?  isIFrame : 0;

  PTRACE(4, "THEORA", "Encap\tConfig Data in queue for RTP frame:  " << _packedConfigData.len << ", position: "<< _packedConfigData.pos);
  PTRACE(4, "THEORA", "Encap\tFrame Data in queue for RTP frame:  " << _encodedData.len << ", position: "<< _encodedData.pos);

  if ((!_configSent) || (_packedConfigData.pos > 0))  // config not yet sent or still fragments left
    assembleRTPFrame (frame, _packedConfigData, true);
  else if (_encodedData.len > 0)
    assembleRTPFrame (frame, _encodedData, false);
  else 
    PTRACE(1, "THEORA", "Encap\tNeither config data nor frame data to send");

  if (frame.GetMarker()) {
    flags |= isLastFrame;  // marker bit on last frame of video
    _encodedData.len = 0;
    _encodedData.pos = 0;
  }
}
bool theoraFrame::disassembleRTPFrame(PluginCodec_RTP & frame, data_t & frameData, bool isPackedConfig)
{
  uint8_t* dataPtr = frame.GetPayloadPtr();
  theoraFT fragmentType = (theoraFT)((dataPtr[3] & 0xC0) >> 6); 
  uint16_t numberPackets = dataPtr[3] & 0x0F; 
  uint32_t len;
  uint32_t currentIdent = (dataPtr[0] << 16) + (dataPtr[1] << 8) + dataPtr[2]; 
  uint16_t i;

  _packets.clear();

  len = (dataPtr[4] << 8) + dataPtr[5];
  if (len > frame.GetPayloadSize() - 6) {
    PTRACE(1, "THEORA", "Deencap\tMalformed packet received, indicated payload length of " << len << " bytes but packet payload is only " << frame.GetPayloadSize() - 6 << "bytes long");
    return false;
  }

  switch (fragmentType) {
    case NOT_FRAGMENTED:  
      frameData.pos = 0;
      frameData.len = 0;
      dataPtr += 4;

      if ((isPackedConfig) && (numberPackets > 1)) {
        PTRACE(1, "THEORA", "Deencap\tPacked config with " << numberPackets << " > 1 makes no sense - taking only first packet");
        numberPackets = 1;
      }

      for (i=1; i <= numberPackets; i++) {

        len = (dataPtr[0] << 8) + dataPtr[1];
        if ((frameData.len + (i<<1) + 4 + len) > frame.GetPayloadSize()) {
          PTRACE(1, "THEORA", "Deencap\tMalformed packet, packet #" << i << " has length larger than total packet length");
          return false;
        }
        if (frameData.pos + len > MAX_FRAME_SIZE) {
         PTRACE(1, "THEORA", "Deencap\tCannot add packet to buffer since buffer is only " << MAX_FRAME_SIZE << " bytes long and " << frameData.pos + len << " bytes are needed");
          return false;
        }

        memcpy (frameData.ptr + frameData.pos, dataPtr + 2, len);

        if (isPackedConfig) {
          _ident = currentIdent;
         } 
         else {
          packet_t packet;
          packet.pos = frameData.pos;
          packet.len = len;
          _packets.push_back(packet);
        }

        frameData.pos += len;
        frameData.len += len;

        if ((i+1) <= numberPackets) {
          if ((frameData.len + ((i+1) << 1) + 6) > frame.GetPayloadSize()) {
            PTRACE(1, "THEORA", "Deencap\tMalformed packet, packet #" << i + 1 << " has length field outside packet");
            return false;
          }
          dataPtr += len + 2;
        }

        PTRACE(4, "THEORA", "Deencap\tAdded unfragmented segment #" << i << " of size " << len << " to data block of " << frameData.len << ", pos: " << frameData.pos);
      }
      return true;

    case START_FRAGMENT:
      if (len > MAX_FRAME_SIZE) {
        PTRACE(1, "THEORA", "Deencap\tCannot add continuation packet to buffer since buffer is only " << MAX_FRAME_SIZE << " bytes long and " << frameData.pos + len << " bytes are needed");
      }
      frameData.pos = 0;
      frameData.len = 0;
      memcpy (frameData.ptr + frameData.pos, dataPtr + 6, len);
      frameData.pos = len;
      PTRACE(4, "THEORA", "Deencap\tAdded start fragment of size " << len << " to data block of " << frameData.len << ", pos: " << frameData.pos);
      return true;

    case CONTINUATION_FRAGMENT:
      if (frameData.pos + len > MAX_FRAME_SIZE) {
        PTRACE(1, "THEORA", "Deencap\tCannot add continuation packet to buffer since buffer is only " << MAX_FRAME_SIZE << " bytes long and " << frameData.pos + len << " bytes are needed");
      }
      if (frameData.pos == 0) { PTRACE(1, "THEORA", "Received fragment continuation when fragment start was never received"); return false;}
      memcpy (frameData.ptr + frameData.pos, dataPtr + 6, len);
      frameData.pos +=len;
      frameData.len = 0;
      PTRACE(4, "THEORA", "Deencap\tAdded continuation fragment of size " << len << " to data block of " << frameData.len << ", pos: " << frameData.pos);
      return true;
    case END_FRAGMENT:
      if (frameData.pos + len > MAX_FRAME_SIZE) {
        PTRACE(1, "THEORA", "Deencap\tCannot add continuation packet to buffer since buffer is only " << MAX_FRAME_SIZE << " bytes long and " << frameData.pos + len << " bytes are needed");
      }
      if (frameData.pos == 0) { PTRACE(1, "THEORA", "Received fragment end when fragment start was never received"); return false;}
      memcpy (frameData.ptr + frameData.pos, dataPtr + 6, len);
      frameData.pos += len;
      frameData.len = frameData.pos;
      if (isPackedConfig)  {
        _ident = currentIdent;
      }
      else {
        packet_t packet;
        packet.pos = 0;
        packet.len = frameData.len;
        _packets.push_back(packet);
      }
      PTRACE(4, "THEORA", "Deencap\tAdded end fragment of size " << len << " to data block of " << frameData.len << ", pos: " << frameData.pos);
      return true;
    default:
      PTRACE(1, "THEORA", "Deencap\tIgnoring unknown fragment type " << fragmentType);
      return false;
  }
  return false;
}

bool theoraFrame::SetFromRTPFrame(PluginCodec_RTP & frame, unsigned int & /*flags*/) {

  // packet too short
  if (frame.GetPayloadSize() < 6) {
    PTRACE(1, "THEORA", "Deencap\tPacket too short, RTP payload length < 6 bytes"); 
    return false;
  } 

  uint8_t* dataPtr = frame.GetPayloadPtr();
  theoraTDT dataType = (theoraTDT)(( dataPtr[3] & 0x30) >> 4); 
  uint32_t currentIdent = (dataPtr[0] << 16) + (dataPtr[1] << 8) + dataPtr[2]; 

  switch (dataType) {
    case RAW_THEORA_PAYLOAD:
      PTRACE(4, "THEORA", "Deencap\tDeencapsulating raw theora payload packet"); 
      return disassembleRTPFrame(frame, _encodedData, false);
    case THEORA_PACKED_CONFIG_PAYLOAD: //config
      PTRACE(4, "THEORA", "Deencap\tDeencapsulating packed config payload packet"); 
      if (currentIdent != _ident) {
        return disassembleRTPFrame(frame, _packedConfigData, true);
       }
       else {
        PTRACE(4, "THEORA", "Deencap\tPacked config is already known for this stream - ignoring packet"); 
        return true;
      }
    case LEGACY_THEORA_COMMENT_PAYLOAD:
      PTRACE(1, "THEORA", "Deencap\tIgnored packet with legacy theora comment payload"); 
      return true;
    case RESERVED:
      PTRACE(1, "THEORA", "Deencap\tIgnored packet with reserved payload"); 
      return true;
    default:
      PTRACE(1, "THEORA", "Deencap\tIgnored packet with unknown payload " << dataType); 
      return false;
  }
  return false;
}

bool theoraFrame::HasOggPackets () {
  return (_encodedData.len  || _packedConfigData.len);
}

void theoraFrame::GetOggPacket(ogg_packet* oggPacket) {
  oggPacket->e_o_s = 0;
  oggPacket->granulepos = 0;
  oggPacket->packetno = 0;

  if (_packedConfigData.len > 0) {
    oggPacket->b_o_s = 1;
    if (_headerReturned) {
      oggPacket->bytes = _packedConfigData.len - THEORA_HEADER_PACKET_SIZE;
      oggPacket->packet = _packedConfigData.ptr + THEORA_HEADER_PACKET_SIZE;
      _headerReturned = false;
      _packedConfigData.len = 0;
    } else {
      oggPacket->bytes = THEORA_HEADER_PACKET_SIZE;
      oggPacket->packet = _packedConfigData.ptr;
      _headerReturned = true;
    }
  } 
  else if ((_encodedData.len > 0) && (!_packets.empty())) {
    packet_t packet;
    packet = _packets.front();
    oggPacket->bytes = packet.len;
    oggPacket->packet = _encodedData.ptr + packet.pos;
    oggPacket->b_o_s = 0;
    _packets.erase(_packets.begin());
    if (_packets.empty()) {
      _encodedData.len = 0;
      _encodedData.pos = 0;
    }
  } else {
    oggPacket->bytes = 0;
    oggPacket->packet = NULL;
  }
}

