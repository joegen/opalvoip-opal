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
 * Revision 1.2002  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "vidcodec.h"
#endif

#include <codec/vidcodec.h>

#include <ptlib/vconvert.h>


OpalMediaFormat const OpalRGB24(
  OPAL_RGB24,
  OpalMediaFormat::DefaultVideoSessionID,
  RTP_DataFrame::DynamicBase,
  OPAL_RGB24,
  FALSE,   // No jitter
  24*PVideoDevice::CIFWidth*PVideoDevice::CIFHeight*25,  // Bandwith for PAL CIF
  0,
  0,
  OpalMediaFormat::VideoClockRate
);

OpalMediaFormat const OpalRGB32(
  OPAL_RGB32,
  OpalMediaFormat::DefaultVideoSessionID,
  RTP_DataFrame::DynamicBase,
  OPAL_RGB32,
  FALSE,   // No jitter
  32*PVideoDevice::CIFWidth*PVideoDevice::CIFHeight*25,  // Bandwith for PAL CIF
  0,
  0,
  OpalMediaFormat::VideoClockRate
);

OpalMediaFormat const OpalYUV420P(
  OPAL_YUV420P,
  OpalMediaFormat::DefaultVideoSessionID,
  RTP_DataFrame::DynamicBase,
  OPAL_YUV420P,
  FALSE,   // No jitter
  12*PVideoDevice::CIFWidth*PVideoDevice::CIFHeight*25,  // Bandwith for PAL CIF
  0,
  0,
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

H323_UncompVideoCapability::H323_UncompVideoCapability(H323EndPoint & endpoint,
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
  converter = NULL;
}


OpalUncompVideoTranscoder::~OpalUncompVideoTranscoder()
{
  delete converter;
}


PINDEX OpalUncompVideoTranscoder::GetOptimalDataFrameSize(BOOL input) const
{
  if (converter == NULL)
    return 0;

  return input ? converter->GetMaxSrcFrameBytes() : converter->GetMaxDstFrameBytes();
}


BOOL OpalUncompVideoTranscoder::ConvertFrames(const RTP_DataFrame & input,
                                              RTP_DataFrameList & output)
{
  output.RemoveAll();

  const FrameHeader * srcHeader = (const FrameHeader *)input.GetPayloadPtr();
  if (srcHeader->x != 0 || srcHeader->y != 0)
    return FALSE;

  if (srcHeader->width != frameWidth || srcHeader->height != frameHeight) {
    delete converter;
    converter = NULL;
    frameWidth = srcHeader->width;
    frameHeight = srcHeader->height;
  }

  if (converter == NULL) {
    converter = PColourConverter::Create(GetInputFormat(), GetOutputFormat(),
                                         frameWidth, frameHeight);
    if (converter == NULL)
      return FALSE;
  }

  // Calculate number of output frames
  PINDEX bytesPerScanLine = converter->GetMaxDstFrameBytes()/frameHeight;
  PINDEX scanLinePerBand = converter->GetMaxDstFrameBytes()/maxOutputPayloadSize/bytesPerScanLine;
  PINDEX bandCount = (frameHeight+scanLinePerBand-1)/scanLinePerBand;

  if (bandCount == 1) {
    RTP_DataFrame * pkt = new RTP_DataFrame(converter->GetMaxDstFrameBytes());
    pkt->SetPayloadType(outputMediaFormat.GetPayloadType());
    pkt->SetTimestamp(input.GetTimestamp());
    output.Append(pkt);
    FrameHeader * dstHeader = (FrameHeader *)pkt->GetPayloadPtr();
    *dstHeader = *srcHeader;
    return converter->Convert(srcHeader->data, dstHeader->data);
  }

  // More here
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
