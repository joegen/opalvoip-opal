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
 * Revision 1.6  2007/08/03 07:21:02  csoutheren
 * Remove warnings
 *
 * Revision 1.5  2007/07/05 06:36:22  rjongbloed
 * Fixed MSVC compiler warning.
 *
 * Revision 1.4  2007/07/05 06:25:13  rjongbloed
 * Fixed GNU compiler warnings.
 *
 * Revision 1.3  2007/06/30 14:00:05  dsandras
 * Fixed previous commit so that things at least compile. Untested.
 *
 * Revision 1.2  2007/06/29 23:24:25  csoutheren
 * More RFC4175 implementation
 *
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

#define   REASONABLE_UDP_PACKET_SIZE  1000

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
  // make sure the incoming frame is big enough for a frame header
  if (input.GetPayloadSize() < (int)(sizeof(PluginCodec_Video_FrameHeader))) {
    PTRACE(1,"RFC4175\tPayload of grabbed frame too small for frame header");
    return FALSE;
  }

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
  PINDEX linesPerPacket = REASONABLE_UDP_PACKET_SIZE / frameWidthInBytes;

  // if a scan line is longer than a reasonable packet, then return error for now
  if (linesPerPacket <= 0) {
    PTRACE(1,"RFC4175\tframe width too large");
    return FALSE;
  }

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

      // scan line length
      *(PUInt16b *)ptr = (WORD)(PixelsToBytes(frameWidth));  
      ptr += 2;

      // line number +  field flag
      *(PUInt16b *)ptr = (WORD)((y+j) & 0x7fff); 
      ptr += 2;

      // pixel offset of scanline start
      *(PUInt16b *)ptr = (WORD)PixelsToBytes(offset) & ((j == (lineCount-1)) ? 0x8000 : 0x0000); 
      ptr += 2;

      // move to next scan line
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
  Initialise();
}

BOOL OpalRFC4175Decoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & /*output*/)
{
  if (input.GetPayloadSize() < 8) {
    PTRACE(1,"RFC4175\tinput frame too small for header");
    return FALSE;
  }

  // get pointer to scanline table
  BYTE * ptr = input.GetPayloadPtr() + 2;

  BOOL lastLine = FALSE;
  PINDEX firstLineLength = 0;
  BOOL firstLine = TRUE;
  PINDEX lineCount = 0;
  PINDEX maxLineNumber = 0;

  do {

    // ensure there is enough payload for this header
    if ((2 + ((lineCount+1)*6)) >= input.GetPayloadSize()) {
      PTRACE(1,"RFC4175\tinput frame too small for scan line table");
      return FALSE;
    }

    // scan line length
    PINDEX lineLength = BytesToPixels(*(PUInt16b *)ptr);  
    ptr += 2;

    // line number 
    WORD lineNumber = ((*(PUInt16b *)ptr) & 0x7fff); 
    ptr += 2;

    // pixel offset of scanline start
    WORD offset = *(PUInt16b *)ptr;
    ptr += 2;

    // detect if last scanline in table
    if (offset & 0x8000) {
      lastLine = TRUE;
      offset &= 0x7fff;
    }

    // we don't handle partial lines or variable length lines
    if (offset != 0) {
      PTRACE(1,"RFC4175\tpartial lines not supported");
      return FALSE;
    } else if (firstLine) {
      firstLineLength = lineLength;
      firstLine = FALSE;
    } else if (lineLength != firstLineLength) {
      PTRACE(1,"RFC4175\tline length changed during frame");
      return FALSE;
    }

    // keep track of max line number
    if (lineNumber > maxLineNumber)
      maxLineNumber = lineNumber;

    // count lines
    ++lineCount;

  } while (!lastLine);

  // if this is the first frame, allocate the destination frame
  if (firstFrame) {
  }

  return FALSE;
}

BOOL OpalRFC4175Decoder::Initialise()
{
  firstFrame = TRUE;
  width      = 0;
  maxY       = 0;
  return TRUE;
}

#endif // OPAL_RFC4175

