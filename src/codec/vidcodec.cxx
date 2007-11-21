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
 * $Id$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "vidcodec.h"
#endif

#include <opal/buildopts.h>

#if OPAL_VIDEO

#include <codec/vidcodec.h>

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>

#define FRAME_WIDTH  PVideoDevice::CIF16Width
#define FRAME_HEIGHT PVideoDevice::CIF16Height
#define FRAME_RATE   30 // NTSC

const OpalVideoFormat & GetOpalRGB24()
{
  static const OpalVideoFormat RGB24(
    OPAL_RGB24,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    24*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return RGB24;
}

const OpalVideoFormat & GetOpalRGB32()
{
  static const OpalVideoFormat RGB32(
    OPAL_RGB32,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    32*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return RGB32;
}

const OpalVideoFormat & GetOpalYUV420P()
{
  static const OpalVideoFormat YUV420P(
    OPAL_YUV420P,
    RTP_DataFrame::MaxPayloadType,
    NULL,
    FRAME_WIDTH, FRAME_HEIGHT,
    FRAME_RATE,
    12*FRAME_WIDTH*FRAME_HEIGHT*FRAME_RATE  // Bandwidth
  );
  return YUV420P;
}


#define new PNEW

/////////////////////////////////////////////////////////////////////////////

OpalVideoTranscoder::OpalVideoTranscoder(const OpalMediaFormat & inputMediaFormat,
                                         const OpalMediaFormat & outputMediaFormat)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
  , forceIFrame(false)
{
  UpdateOutputMediaFormat(outputMediaFormat);
  fillLevel = 5;
}


BOOL OpalVideoTranscoder::UpdateOutputMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(updateMutex);

  if (!OpalTranscoder::UpdateOutputMediaFormat(mediaFormat))
    return FALSE;

  return TRUE;
}


BOOL OpalVideoTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    forceIFrame = true; // Reset when I-Frame is sent
    return TRUE;
  }

  return OpalTranscoder::ExecuteCommand(command);
}


BOOL OpalVideoTranscoder::Convert(const RTP_DataFrame & /*input*/,
                                  RTP_DataFrame & /*output*/)
{
  return FALSE;
}

PString OpalVideoUpdatePicture::GetName() const
{
  return "Update Picture";
}

PString OpalTemporalSpatialTradeOff::GetName() const
{
  return "Temporal Spatial Trade Off";
}

PString OpalLostPartialPicture::GetName() const
{
  return "Lost Partial Picture";
}

PString OpalLostPicture::GetName() const
{
  return "Lost Picture";
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

H323_UncompVideoCapability::H323_UncompVideoCapability(const H323EndPoint & /*endpoint*/,
                                                       const PString & colourFmt)
  : H323NonStandardVideoCapability((const BYTE *)(const char *)colourFmt,
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

#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

OpalUncompVideoTranscoder::OpalUncompVideoTranscoder(const OpalMediaFormat & inputMediaFormat,
                                                     const OpalMediaFormat & outputMediaFormat)
  : OpalVideoTranscoder(inputMediaFormat, outputMediaFormat)
{
}


OpalUncompVideoTranscoder::~OpalUncompVideoTranscoder()
{
}


PINDEX OpalUncompVideoTranscoder::GetOptimalDataFrameSize(BOOL input) const
{
#ifndef NO_OPAL_VIDEO
  PINDEX frameBytes = PVideoDevice::CalculateFrameBytes(frameWidth, frameHeight, GetOutputFormat());
  if (!input && frameBytes > maxOutputSize)
    frameBytes = maxOutputSize;

  return frameBytes;
#else
  return 0;
#endif
}


BOOL OpalUncompVideoTranscoder::ConvertFrames(const RTP_DataFrame & input,
                                              RTP_DataFrameList & output)
{
#ifndef NO_OPAL_VIDEO
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
    memcpy(OPAL_VIDEO_FRAME_DATA_PTR(dstHeader), OPAL_VIDEO_FRAME_DATA_PTR(srcHeader)+band*bytesPerScanLine, scanLinesPerBand*bytesPerScanLine);
  }

  output[output.GetSize()-1].SetMarker(TRUE);

  return TRUE;
#else
  return FALSE;
#endif
}

#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////
