/*
 * rfc4175.cxx
 *
 * RFC4175 transport for uncompressed video
 *
 * Open Phone Abstraction Library
 *
 * Copyright (C) 2007 Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: rfc4175.cxx,v $
 * Revision 1.1  2007/05/31 14:11:45  csoutheren
 * Add initial support for RFC 4175 uncompressed video encoding
 *
 */

#include <ptlib.h>

#include <ptclib/random.h>

#include <opal/buildopts.h>

#if OPAL_RFC4175

#include <codec/rfc4175.h>
#include <codec/opalplugin.h>

namespace PWLibStupidLinkerHacks {
  int rfc4175Loader;
};

OPAL_REGISTER_RFC4175_VIDEO(RGB24)
OPAL_REGISTER_RFC4175_VIDEO(YUV420P)

#define   FRAME_WIDTH   1920
#define   FRAME_HEIGHT  1080
#define   FRAME_RATE    60

const OpalVideoFormat & GetOpalRFC4175_YUV420P()
{
  static const OpalVideoFormat RFC4175YUV420P(
    OPAL_RFC4175_YUV420P,
    RTP_DataFrame::MaxPayloadType,
    OPAL_RFC4175_YUV420P,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    0xffffffff  //12*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return RFC4175YUV420P;
}

const OpalVideoFormat & GetOpalRFC4175_RGB24()
{
  static const OpalVideoFormat RFC4175RGB24(
    OPAL_RFC4175_RGB24,
    RTP_DataFrame::MaxPayloadType,
    OPAL_RFC4175_RGB24,
    32767, 32767,
    FRAME_RATE,
    0xffffffff  //24*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return RFC4175RGB24;
}

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Transcoder::OpalRFC4175Transcoder(      
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat  ///<  Output media format
)
 : OpalVideoTranscoder(inputMediaFormat, outputMediaFormat)
{
}

PINDEX OpalRFC4175Transcoder::RFC4175HeaderSize(PINDEX lines)
{ return 2 + lines*6; }

PINDEX OpalRFC4175Transcoder::GetOptimalDataFrameSize(BOOL /*input*/) const
{
  return 0;
}

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Encoder::OpalRFC4175Encoder(      
  const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
  const OpalMediaFormat & outputMediaFormat  ///<  Output media format
) : OpalRFC4175Transcoder(inputMediaFormat, outputMediaFormat)
{
  extendedSequenceNumber = PRandom::Number();
}

BOOL OpalRFC4175Encoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output)
{
  PluginCodec_Video_FrameHeader * header = (PluginCodec_Video_FrameHeader *)input.GetPayloadPtr();
  if (header->x != 0 && header->y != 0) {
    PTRACE(1,"RFC4175\tVideo grab of partial frame unsupported");
    return FALSE;
  }

  // get information from frame header
  PINDEX frameHeight       = header->height;
  PINDEX frameWidth        = header->width;
  PINDEX frameWidthInBytes = PixelsToBytes(frameWidth);

  // make sure the incoming frame is big enough for the specified frame size
  if (input.GetPayloadSize() < (int)(sizeof(PluginCodec_Video_FrameHeader) + frameHeight*frameWidthInBytes)) {
    PTRACE(1,"RFC4175\tPayload of grabbed frame too small for full frame");
    return FALSE;
  }

  // calculate how many scan lines will fit in a reasonable UDP packet
  PINDEX linesPerPacket = 1000 / frameWidthInBytes;

  // encode the scan lines
  PINDEX y = 0;
  while (y < frameHeight) {

    // allocate a new output frame
    RTP_DataFrame * frame = new RTP_DataFrame;
    output.Append(frame);

    // populate RTP fields
    frame->SetTimestamp(input.GetTimestamp());
    frame->SetSequenceNumber((WORD)(extendedSequenceNumber & 0xffff));

    // calculate number of scanlines in this packet
    PINDEX lineCount = PMIN(linesPerPacket, frameHeight-y);

    // set size of the packet
    frame->SetPayloadSize(RFC4175HeaderSize(lineCount) + lineCount*frameWidthInBytes);

    // populate extended sequence number
    *(PUInt16b *)frame->GetPayloadPtr() = (WORD)(extendedSequenceNumber >> 16);

    // initialise scan table
    BYTE * ptr = frame->GetPayloadPtr() + 2;
    PINDEX j;
    PINDEX offset = 0;
    for (j = 0; j < lineCount; ++j) {
      *(PUInt16b *)ptr = (WORD)(y+j); // field identifier here
      ptr += 2;
      *(PUInt16b *)ptr = (WORD)PixelsToBytes(offset) & ((j == (lineCount-1)) ? 0x8000 : 0x0000);
      ptr += 2;
      *(PUInt16b *)ptr = (WORD)(PixelsToBytes(frameWidth) & 0x7fff);
      ptr += 2;
      offset += frameWidth;
    }

    // copy scan line data
    memcpy(ptr, OPAL_VIDEO_FRAME_DATA_PTR(header)+y*frameWidthInBytes, lineCount*frameWidthInBytes);

    // move to next block of scan lines
    y += lineCount;

    // increment sequence number
    ++extendedSequenceNumber;
  }

  // set marker bit in last packet, if any packets created
  if (output.GetSize() > 0)
    output[output.GetSize()-1].SetMarker(TRUE);

  return FALSE;
}

/////////////////////////////////////////////////////////////////////////////

OpalRFC4175Decoder::OpalRFC4175Decoder(      
  const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
  const OpalMediaFormat & outputMediaFormat  ///<  Output media format
) : OpalRFC4175Transcoder(inputMediaFormat, outputMediaFormat)
{
}

BOOL OpalRFC4175Decoder::ConvertFrames(const RTP_DataFrame & /*input*/, RTP_DataFrameList & /*output*/)
{
  return FALSE;
}

#endif // OPAL_RFC4175

