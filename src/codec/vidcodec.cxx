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
 * Revision 2.22  2007/09/25 09:49:54  rjongbloed
 * Fixed videoFastUpdate, is not a count but a simple boolean.
 *
 * Revision 2.21  2007/09/09 23:46:28  rjongbloed
 * There is no encoding name for internal formats
 *
 * Revision 2.20  2007/08/31 07:56:19  dsandras
 * Applied patch from Matthias Schneider <ma30002000 yahoo de> :
 * * allow higher resolutions
 * * code cleanup
 * Thanks!
 *
 * Revision 2.19  2007/08/17 11:14:17  csoutheren
 * Add OnLostPicture and OnLostPartialPicture commands
 *
 * Revision 2.18  2007/04/10 05:15:54  rjongbloed
 * Fixed issue with use of static C string variables in DLL environment,
 *   must use functional interface for correct initialisation.
 *
 * Revision 2.17  2006/11/01 06:57:23  csoutheren
 * Fixed usage of YUV frame header
 *
 * Revision 2.16  2006/09/22 00:58:41  csoutheren
 * Fix usages of PAtomicInteger
 *
 * Revision 2.15  2006/07/24 14:03:39  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.11.4.2  2006/04/19 07:52:30  csoutheren
 * Add ability to have SIP-only and H.323-only codecs, and implement for H.261
 *
 * Revision 2.11.4.1  2006/03/23 07:55:18  csoutheren
 * Audio plugin H.323 capability merging completed.
 * GSM, LBC, G.711 working. Speex and LPC-10 are not
 *
 * Revision 2.14  2006/06/30 06:49:31  csoutheren
 * Fixed disable of video code to use correct #define
 *
 * Revision 2.13  2006/06/30 01:05:50  csoutheren
 * Applied 1509203 - Fix compilation when video is disabled
 * Thanks to Boris Pavacic
 *
 * Revision 2.12  2006/06/21 04:54:15  csoutheren
 * Fixed build with latest PWLib
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


PBoolean OpalVideoTranscoder::UpdateOutputMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(updateMutex);

  if (!OpalTranscoder::UpdateOutputMediaFormat(mediaFormat))
    return PFalse;

  return PTrue;
}


PBoolean OpalVideoTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    forceIFrame = true; // Reset when I-Frame is sent
    return PTrue;
  }

  return OpalTranscoder::ExecuteCommand(command);
}


PBoolean OpalVideoTranscoder::Convert(const RTP_DataFrame & /*input*/,
                                  RTP_DataFrame & /*output*/)
{
  return PFalse;
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


PINDEX OpalUncompVideoTranscoder::GetOptimalDataFrameSize(PBoolean input) const
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


PBoolean OpalUncompVideoTranscoder::ConvertFrames(const RTP_DataFrame & input,
                                              RTP_DataFrameList & output)
{
#ifndef NO_OPAL_VIDEO
  output.RemoveAll();

  const FrameHeader * srcHeader = (const FrameHeader *)input.GetPayloadPtr();
  if (srcHeader->x != 0 || srcHeader->y != 0)
    return PFalse;

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
    return PFalse;

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

  output[output.GetSize()-1].SetMarker(PTrue);

  return PTrue;
#else
  return PFalse;
#endif
}

#endif // OPAL_VIDEO

/////////////////////////////////////////////////////////////////////////////
