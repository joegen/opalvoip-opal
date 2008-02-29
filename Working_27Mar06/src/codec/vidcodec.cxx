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
 * Revision 1.2012.4.1  2006/03/23 07:55:18  csoutheren
 * Audio plugin H.323 capability merging completed.
 * GSM, LBC, G.711 working. Speex and LPC-10 are not
 *
 * Revision 2.11  2006/01/12 17:55:22  dsandras
 * Initialize the fillLevel to a saner value.
 *
 * Revision 2.10  2005/10/21 17:58:31  dsandras
 * Applied patch from Hannes Friederich <hannesf AATT ee.ethz.ch> to fix OpalVideoUpdatePicture - PIsDescendant problems. Thanks!
 *
 * Revision 2.9  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.8  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.7  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.6  2005/07/24 07:33:09  rjongbloed
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

#include <opal/buildopts.h>

#if OPAL_VIDEO

#include <codec/vidcodec.h>

#include <ptlib/vconvert.h>

#define FRAME_WIDTH  PVideoDevice::CIFWidth
#define FRAME_HEIGHT PVideoDevice::CIFHeight
#define FRAME_RATE   25 // PAL

const OpalVideoFormat & GetOpalRGB24()
{
  static const OpalVideoFormat RGB24(
    OPAL_RGB24,
    RTP_DataFrame::MaxPayloadType,
    OPAL_RGB24,
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
    OPAL_RGB32,
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
    OPAL_YUV420P,
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
{
  UpdateOutputMediaFormat(outputMediaFormat);
  fillLevel = 5;
  updatePicture = false;
}


BOOL OpalVideoTranscoder::UpdateOutputMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(updateMutex);

  if (!OpalTranscoder::UpdateOutputMediaFormat(mediaFormat))
    return FALSE;

  frameWidth = outputMediaFormat.GetOptionInteger(OpalVideoFormat::FrameWidthOption, PVideoDevice::CIFWidth);
  frameHeight = outputMediaFormat.GetOptionInteger(OpalVideoFormat::FrameHeightOption, PVideoDevice::CIFHeight);
  videoQuality = outputMediaFormat.GetOptionInteger(OpalVideoFormat::EncodingQualityOption, 15);
  targetBitRate = outputMediaFormat.GetOptionInteger(OpalVideoFormat::TargetBitRateOption, 64000);
  dynamicVideoQuality = outputMediaFormat.GetOptionBoolean(OpalVideoFormat::DynamicVideoQualityOption, false);
  adaptivePacketDelay = outputMediaFormat.GetOptionBoolean(OpalVideoFormat::AdaptivePacketDelayOption, false);
  return TRUE;
}


BOOL OpalVideoTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    updatePicture = true;
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

#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////
