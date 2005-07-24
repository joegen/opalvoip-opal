/*
 * vidcodec.cxx
 *
 * Uncompressed video handler
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2003 Equivalence Pty. Ltd.
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
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: vidcodec.cxx,v $
 * Revision 1.2007  2005/07/24 07:33:09  rjongbloed
 * Simplified "uncompressed" transcoder sp can test video media streams.
 *
 * Revision 2.5  2005/02/21 12:19:54  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.4  2004/09/01 12:21:27  rjongbloed
 * Added initialisation of H323EndPoints capability table to be all codecs so can
 *   correctly build remote caps from fqast connect params. This had knock on effect
 *   with const keywords added in numerous places.
 *
 * Revision 2.3  2004/07/11 12:37:00  rjongbloed
 * Changed internal video formats to use value so do not appear in available media lists.
 *
 * Revision 2.2  2004/01/18 15:35:20  rjongbloed
 * More work on video support
 *
 * Revision 2.1  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "vidcodec.h"
#endif

#include <codec/vidcodec.h>

#include <ptlib/vconvert.h>

#define FRAME_WIDTH  PVideoDevice::CIFWidth
#define FRAME_HEIGHT PVideoDevice::CIFHeight
#define FRAME_RATE   25 // PAL

const OpalMediaFormat OpalRGB24(
  OPAL_RGB24,
  OpalMediaFormat::DefaultVideoSessionID,
  RTP_DataFrame::MaxPayloadType,
  OPAL_RGB24,
  FALSE,   // No jitter
  24*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE,  // Bandwidth
  3*FRAME_WIDTH*FRAME_HEIGHT,
  96000/FRAME_RATE,
  OpalMediaFormat::VideoClockRate
);

const OpalMediaFormat OpalRGB32(
  OPAL_RGB32,
  OpalMediaFormat::DefaultVideoSessionID,
  RTP_DataFrame::MaxPayloadType,
  OPAL_RGB32,
  FALSE,   // No jitter
  32*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE,  // Bandwidth
  4*FRAME_WIDTH*FRAME_HEIGHT,
  96000/FRAME_RATE,
  OpalMediaFormat::VideoClockRate
);

const OpalMediaFormat OpalYUV420P(
  OPAL_YUV420P,
  OpalMediaFormat::DefaultVideoSessionID,
  RTP_DataFrame::MaxPayloadType,
  OPAL_YUV420P,
  FALSE,   // No jitter
  12*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE,  // Bandwidth
  3*FRAME_WIDTH*FRAME_HEIGHT/2,
  96000/FRAME_RATE,
  OpalMediaFormat::VideoClockRate
);


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalVideoTranscoder::OpalVideoTranscoder(const OpalTranscoderRegistration & registration)
  : OpalTranscoder(registration)
{
  frameWidth = PVideoDevice::CIFWidth;
  frameHeight = PVideoDevice::CIFHeight;
  fillLevel = 0;
  videoQuality = 9;
}


BOOL OpalVideoTranscoder::Convert(const RTP_DataFrame & /*input*/,
                                  RTP_DataFrame & /*output*/)
{
  return FALSE;
}


///////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

H323_UncompVideoCapability::H323_UncompVideoCapability(const H323EndPoint & endpoint,
                                                       const PString & colourFmt)
  : H323NonStandardVideoCapability(endpoint,
                                   (const BYTE *)(const char *)colourFmt,
                                   colourFmt.GetLength()),
    colourFormat(colourFmt)
{
}


PObject * H323_UncompVideoCapability::Clone() const
{
  return new H323_UncompVideoCapability(*this);
}


PString H323_UncompVideoCapability::GetFormatName() const
{
  return colourFormat;
}

#endif // ifndef NO_H323


/////////////////////////////////////////////////////////////////////////////

OpalUncompVideoTranscoder::OpalUncompVideoTranscoder(const OpalTranscoderRegistration & registration)
  : OpalVideoTranscoder(registration)
{
}


OpalUncompVideoTranscoder::~OpalUncompVideoTranscoder()
{
}


PINDEX OpalUncompVideoTranscoder::GetOptimalDataFrameSize(BOOL input) const
{
  PINDEX frameBytes = PVideoDevice::CalculateFrameBytes(frameWidth, frameHeight, GetOutputFormat());
  if (!input && frameBytes > maxOutputSize)
    frameBytes = maxOutputSize;

  return frameBytes;
}


BOOL OpalUncompVideoTranscoder::ConvertFrames(const RTP_DataFrame & input,
                                              RTP_DataFrameList & output)
{
  output.RemoveAll();

  const FrameHeader * srcHeader = (const FrameHeader *)input.GetPayloadPtr();
  if (srcHeader->x != 0 || srcHeader->y != 0)
    return FALSE;

  if (srcHeader->width != frameWidth || srcHeader->height != frameHeight) {
    frameWidth = srcHeader->width;
    frameHeight = srcHeader->height;
  }

  PINDEX frameBytes = PVideoDevice::CalculateFrameBytes(frameWidth, frameHeight, GetOutputFormat());

  // Calculate number of output frames
  PINDEX bytesPerScanLine = frameBytes/frameHeight;

  unsigned scanLinesPerBand = maxOutputSize/bytesPerScanLine;
  if (scanLinesPerBand > frameHeight)
    scanLinesPerBand = frameHeight;

  unsigned bandCount = (frameHeight+scanLinesPerBand-1)/scanLinesPerBand;
  if (bandCount < 1)
    return FALSE;

  for (unsigned band = 0; band < bandCount; band++) {
    RTP_DataFrame * pkt = new RTP_DataFrame(scanLinesPerBand*bytesPerScanLine);
    pkt->SetPayloadType(outputMediaFormat.GetPayloadType());
    pkt->SetTimestamp(input.GetTimestamp());
    output.Append(pkt);
    FrameHeader * dstHeader = (FrameHeader *)pkt->GetPayloadPtr();
    dstHeader->x = srcHeader->x;
    dstHeader->y = srcHeader->y + band*scanLinesPerBand;
    dstHeader->width = srcHeader->width;
    dstHeader->height = scanLinesPerBand;
    memcpy(dstHeader->data, srcHeader->data+band*bytesPerScanLine, scanLinesPerBand*bytesPerScanLine);
  }

  output[output.GetSize()-1].SetMarker(TRUE);

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
