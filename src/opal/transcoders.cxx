/*
 * transcoders.cxx
 *
 * Abstractions for converting media from one format to another.
 *
 * Open H323 Library
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * $Log: transcoders.cxx,v $
 * Revision 1.2029  2006/12/31 17:00:14  dsandras
 * Do not try transcoding RTP frames if they do not correspond to the formats
 * for which the transcoder was created.
 *
 * Revision 2.27  2006/11/29 06:28:58  csoutheren
 * Add ability call codec control functions on all transcoders
 *
 * Revision 2.26  2006/10/04 06:33:20  csoutheren
 * Add compliant handling for multiples speex frames per packet
 *
 * Revision 2.25  2006/10/03 01:06:35  rjongbloed
 * Fixed GNU compiler compatibility.
 *
 * Revision 2.24  2006/08/11 08:09:24  csoutheren
 * Fix incorrect handling of variable output frame sizes - Speex plugin now works :)
 *
 * Revision 2.23  2006/08/11 07:52:02  csoutheren
 * Fix problem with media format factory in VC 2005
 * Fixing problems with Speex codec
 * Remove non-portable usages of PFactory code
 *
 * Revision 2.22  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.21  2006/06/30 06:49:02  csoutheren
 * Applied 1494416 - Add check for sessionID in transcoder selection
 * Thanks to mturconi
 *
 * Revision 2.20  2006/04/09 12:12:54  rjongbloed
 * Changed the media format option merging to include the transcoder formats.
 *
 * Revision 2.19.2.5  2006/04/24 01:50:00  csoutheren
 * Fixed problem with selecting codecs from wrong sessions
 *
 * Revision 2.19.2.4  2006/04/11 05:12:25  csoutheren
 * Updated to current OpalMediaFormat changes
 *
 * Revision 2.19.2.3  2006/04/10 06:24:30  csoutheren
 * Backport from CVS head up to Plugin_Merge3
 *
 * Revision 2.19.2.2  2006/04/07 07:57:20  csoutheren
 * Halfway through media format changes - not working, but closer
 *
 * Revision 2.19.2.1  2006/04/06 01:21:20  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 2.19  2006/02/08 04:00:19  csoutheren
 * Fixed for G.726 codec
 * Thanks to Michael Tinglof
 *
 * Revision 2.18  2006/02/02 07:02:58  csoutheren
 * Added RTP payload map to transcoders and connections to allow remote SIP endpoints
 * to change the payload type used for outgoing RTP.
 *
 * Revision 2.17  2005/12/30 14:33:12  dsandras
 * Added support for Packet Loss Concealment frames for framed codecs supporting it similarly to what was done for OpenH323.
 *
 * Revision 2.16  2005/09/13 20:48:22  dominance
 * minor cleanups needed to support mingw compilation. Thanks goes to Julien Puydt.
 *
 * Revision 2.15  2005/09/06 12:44:49  rjongbloed
 * Many fixes to finalise the video processing: merging remote media
 *
 * Revision 2.14  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.13  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.12  2005/07/14 08:53:34  csoutheren
 * Change transcoding selection algorithm to prefer untranslated codec connections rather
 * than multi-stage transcoders
 *
 * Revision 2.11  2005/02/17 03:25:05  csoutheren
 * Added support for audio codecs that consume and produce variable size
 * frames, such as G.723.1
 *
 * Revision 2.10  2004/07/11 12:34:49  rjongbloed
 * Added function to get a list of all possible media formats that may be used given
 *   a list of media and taking into account all of the registered transcoders.
 *
 * Revision 2.9  2004/05/24 13:37:32  rjongbloed
 * Fixed propagating marker bit across transcoder which is important for
 *   silence suppression, thanks Ted Szoczei
 *
 * Revision 2.8  2004/03/22 11:32:42  rjongbloed
 * Added new codec type for 16 bit Linear PCM as must distinguish between the internal
 *   format used by such things as the sound card and the RTP payload format which
 *   is always big endian.
 *
 * Revision 2.7  2004/02/16 09:15:20  csoutheren
 * Fixed problems with codecs on Unix systems
 *
 * Revision 2.6  2004/01/18 15:35:21  rjongbloed
 * More work on video support
 *
 * Revision 2.5  2003/12/15 11:56:17  rjongbloed
 * Applied numerous bug fixes, thank you very much Ted Szoczei
 *
 * Revision 2.4  2003/06/02 02:59:24  rjongbloed
 * Changed transcoder search so uses destination list as preference order.
 *
 * Revision 2.3  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.2  2002/02/13 02:30:37  robertj
 * Added ability for media patch (and transcoders) to handle multiple RTP frames.
 *
 * Revision 2.1  2001/08/01 05:46:55  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 * Added functions to aid in determining if a transcoder can be used to get
 *   to another media format.
 * Fixed problem with streamed transcoder used in G.711.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transcoders.h"
#endif

#include <opal/transcoders.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalMediaFormatPair::OpalMediaFormatPair(const OpalMediaFormat & inputFmt,
                                         const OpalMediaFormat & outputFmt)
  : inputMediaFormat(inputFmt),
    outputMediaFormat(outputFmt)
{
}


void OpalMediaFormatPair::PrintOn(ostream & strm) const
{
  strm << inputMediaFormat << "->" << outputMediaFormat;
}


PObject::Comparison OpalMediaFormatPair::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, OpalMediaFormatPair), PInvalidCast);
  const OpalMediaFormatPair & other = (const OpalMediaFormatPair &)obj;
  if (inputMediaFormat < other.inputMediaFormat)
    return LessThan;
  if (inputMediaFormat > other.inputMediaFormat)
    return GreaterThan;
  return outputMediaFormat.Compare(other.outputMediaFormat);
}


/////////////////////////////////////////////////////////////////////////////

OpalTranscoder::OpalTranscoder(const OpalMediaFormat & inputMediaFormat,
                               const OpalMediaFormat & outputMediaFormat)
  : OpalMediaFormatPair(inputMediaFormat, outputMediaFormat)
{
  maxOutputSize = RTP_DataFrame::MaxEthernetPayloadSize;
}


BOOL OpalTranscoder::UpdateOutputMediaFormat(const OpalMediaFormat & mediaFormat)
{
  PWaitAndSignal mutex(updateMutex);

  if (outputMediaFormat != mediaFormat)
    return FALSE;

  outputMediaFormat = mediaFormat;
  outputMediaFormatUpdated = true;
  return TRUE;
}


BOOL OpalTranscoder::ExecuteCommand(const OpalMediaCommand & /*command*/)
{
  return FALSE;
}


BOOL OpalTranscoder::ConvertFrames(const RTP_DataFrame & input,
                                   RTP_DataFrameList & output)
{
  RTP_DataFrame::PayloadTypes pt;

  if (output.IsEmpty())
    output.Append(new RTP_DataFrame);
  else {
    while (output.GetSize() > 1)
      output.RemoveAt(1);
  }

  if (payloadTypeMap.size() == 0)
    output[0].SetPayloadType(outputMediaFormat.GetPayloadType());
  else {
    RTP_DataFrame::PayloadMapType::iterator r = payloadTypeMap.find(outputMediaFormat.GetPayloadType());
    if (r != payloadTypeMap.end())
      output[0].SetPayloadType(r->second);
    else
      output[0].SetPayloadType(outputMediaFormat.GetPayloadType());
  }
  output[0].SetTimestamp(input.GetTimestamp());
  output[0].SetMarker(input.GetMarker());

  if (payloadTypeMap.size() == 0) {
    pt = inputMediaFormat.GetPayloadType();
  }
  else {
    RTP_DataFrame::PayloadMapType::iterator r = payloadTypeMap.find(inputMediaFormat.GetPayloadType());
    if (r != payloadTypeMap.end()) 
      pt = r->second;
    else 
      pt = inputMediaFormat.GetPayloadType();
  }

  if (pt != input.GetPayloadType()) {
    PTRACE(2, "Opal\tExpected payload type " << pt << ", but received " << input.GetPayloadType() << ". Ignoring packet");
    return TRUE;
  }

  return Convert(input, output[0]);
}


OpalTranscoder * OpalTranscoder::Create(const OpalMediaFormat & srcFormat,
                                        const OpalMediaFormat & destFormat)
{
  OpalTranscoder * transcoder = OpalTranscoderFactory::CreateInstance(OpalMediaFormatPair(srcFormat, destFormat));
  if (transcoder != NULL)
    transcoder->UpdateOutputMediaFormat(destFormat);
  return transcoder;
}


BOOL OpalTranscoder::SelectFormats(unsigned sessionID,
                                   const OpalMediaFormatList & srcFormats,
                                   const OpalMediaFormatList & dstFormats,
                                   OpalMediaFormat & srcFormat,
                                   OpalMediaFormat & dstFormat)
{
  PINDEX s, d;

  // Search through the supported formats to see if can pass data
  // directly from the given format to a possible one with no transcoders.
  for (d = 0; d < dstFormats.GetSize(); d++) {
    dstFormat = dstFormats[d];
    if (dstFormat.GetDefaultSessionID() == sessionID) {
      for (s = 0; s < srcFormats.GetSize(); s++) {
        srcFormat = srcFormats[s];
        if (srcFormat == dstFormat)
          return srcFormat.Merge(dstFormat) && dstFormat.Merge(srcFormat);
      }
    }
  }

  // Search for a single transcoder to get from a to b
  for (d = 0; d < dstFormats.GetSize(); d++) {
    dstFormat = dstFormats[d];
    if (dstFormat.GetDefaultSessionID() == sessionID) {
      for (s = 0; s < srcFormats.GetSize(); s++) {
        srcFormat = srcFormats[s];
        if (srcFormat.GetDefaultSessionID() == sessionID) {
          OpalMediaFormatPair search(srcFormat, dstFormat);
          OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
          for (OpalTranscoderIterator i = availableTranscoders.begin(); i != availableTranscoders.end(); ++i) {
            OpalMediaFormatPair & target = *i;
            if (search == target)
              return srcFormat.Merge(target.GetInputFormat()) &&
                     dstFormat.Merge(target.GetOutputFormat()) &&
                     srcFormat.Merge(dstFormat);
          }
        }
      }
    }
  }

  // Last gasp search for a double transcoder to get from a to b
  for (d = 0; d < dstFormats.GetSize(); d++) {
    dstFormat = dstFormats[d];
    if (dstFormat.GetDefaultSessionID() == sessionID) {
      for (s = 0; s < srcFormats.GetSize(); s++) {
        srcFormat = srcFormats[s];
        if (srcFormat.GetDefaultSessionID() == sessionID) {
          OpalMediaFormat intermediateFormat;
          if (FindIntermediateFormat(srcFormat, dstFormat, intermediateFormat))
            return TRUE;
        }
      }
    }
  }

  return FALSE;
}


BOOL OpalTranscoder::FindIntermediateFormat(OpalMediaFormat & srcFormat,
                                            OpalMediaFormat & dstFormat,
                                            OpalMediaFormat & intermediateFormat)
{
  intermediateFormat = OpalMediaFormat();

  OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
  for (OpalTranscoderIterator find1 = availableTranscoders.begin(); find1 != availableTranscoders.end(); ++find1) {
    if (find1->GetInputFormat() == srcFormat) {
      for (OpalTranscoderIterator find2 = availableTranscoders.begin(); find2 != availableTranscoders.end(); ++find2) {
        if (find2->GetInputFormat() == find1->GetOutputFormat() && find2->GetOutputFormat() == dstFormat) {
          intermediateFormat = find1->GetOutputFormat();
          intermediateFormat.Merge(find2->GetInputFormat());
          return srcFormat.Merge(find1->GetInputFormat()) &&
                 dstFormat.Merge(find2->GetOutputFormat()) &&
                 srcFormat.Merge(dstFormat);
        }
      }
    }
  }

  return FALSE;
}


OpalMediaFormatList OpalTranscoder::GetDestinationFormats(const OpalMediaFormat & srcFormat)
{
  OpalMediaFormatList list;

  OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
  for (OpalTranscoderIterator find = availableTranscoders.begin(); find != availableTranscoders.end(); ++find) {
    if (find->GetInputFormat() == srcFormat)
      list += find->GetOutputFormat();
  }

  return list;
}


OpalMediaFormatList OpalTranscoder::GetSourceFormats(const OpalMediaFormat & dstFormat)
{
  OpalMediaFormatList list;

  OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
  for (OpalTranscoderIterator find = availableTranscoders.begin(); find != availableTranscoders.end(); ++find) {
    PString s = find->GetInputFormat();
    PString d = find->GetOutputFormat();
    if (find->GetOutputFormat() == dstFormat)
      list += find->GetInputFormat();
  }

  return list;
}


OpalMediaFormatList OpalTranscoder::GetPossibleFormats(const OpalMediaFormatList & formats)
{
  OpalMediaFormatList possibleFormats;

  // Run through the formats connection can do directly and calculate all of
  // the possible formats, including ones via a transcoder
  for (PINDEX f = 0; f < formats.GetSize(); f++) {
    OpalMediaFormat format = formats[f];
    possibleFormats += format;
    OpalMediaFormatList srcFormats = GetSourceFormats(format);
    for (PINDEX i = 0; i < srcFormats.GetSize(); i++) {
      if (GetDestinationFormats(srcFormats[i]).GetSize() > 0)
        possibleFormats += srcFormats[i];
    }
  }

  return possibleFormats;
}


/////////////////////////////////////////////////////////////////////////////

OpalFramedTranscoder::OpalFramedTranscoder(const OpalMediaFormat & inputMediaFormat,
                                           const OpalMediaFormat & outputMediaFormat,
                                           PINDEX inputBytes, PINDEX outputBytes)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
{
  inputBytesPerFrame = inputBytes;
  outputBytesPerFrame = outputBytes;
  //partialFrame = inputBytes;
  //partialBytes = 0;
}


PINDEX OpalFramedTranscoder::GetOptimalDataFrameSize(BOOL input) const
{
  return input ? inputBytesPerFrame : outputBytesPerFrame;
}


BOOL OpalFramedTranscoder::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
{
  const BYTE * inputPtr = input.GetPayloadPtr();
  PINDEX inputLength = input.GetPayloadSize();

  if (inputLength == 0) {
    output.SetPayloadSize(outputBytesPerFrame);
    return ConvertSilentFrame (output.GetPayloadPtr());
  }

  // set maximum output payload size
  if (!output.SetPayloadSize((inputLength + inputBytesPerFrame - 1)/inputBytesPerFrame*outputBytesPerFrame))
    return FALSE;

  BYTE * outputPtr = output.GetPayloadPtr();

  PINDEX outLen = 0;

  while (inputLength > 0) {
    PINDEX consumed = inputLength; // PMIN(inputBytesPerFrame, inputLength);
    PINDEX created  = output.GetPayloadSize() - outLen;

    if (!ConvertFrame(inputPtr, consumed, outputPtr, created))
      return FALSE;

    outputPtr   += created;
    outLen      += created;
    inputPtr    += consumed;
    inputLength -= consumed;
  }

  // set actual output payload size
  output.SetPayloadSize(outLen);

  return TRUE;
}

BOOL OpalFramedTranscoder::ConvertFrame(const BYTE * inputPtr, PINDEX & /*consumed*/, BYTE * outputPtr, PINDEX & /*created*/)
{
  return ConvertFrame(inputPtr, outputPtr);
}

BOOL OpalFramedTranscoder::ConvertFrame(const BYTE * /*inputPtr*/, BYTE * /*outputPtr*/)
{
  return FALSE;
}

BOOL OpalFramedTranscoder::ConvertSilentFrame(BYTE *dst)
{
  memset(dst, 0, outputBytesPerFrame);
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

OpalStreamedTranscoder::OpalStreamedTranscoder(const OpalMediaFormat & inputMediaFormat,
                                               const OpalMediaFormat & outputMediaFormat,
                                               unsigned inputBits,
                                               unsigned outputBits,
                                               PINDEX optimal)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
{
  inputBitsPerSample = inputBits;
  outputBitsPerSample = outputBits;
  optimalSamples = optimal;
}


PINDEX OpalStreamedTranscoder::GetOptimalDataFrameSize(BOOL input) const
{
  return ((input ? inputBitsPerSample : outputBitsPerSample)+7)/8 * optimalSamples;
}


BOOL OpalStreamedTranscoder::Convert(const RTP_DataFrame & input,
                                     RTP_DataFrame & output)
{
  PINDEX i;

  const BYTE * inputBytes = input.GetPayloadPtr();
  const short * inputWords = (const short *)inputBytes;

  BYTE * outputBytes = output.GetPayloadPtr();
  short * outputWords = (short *)outputBytes;

  PINDEX samples = input.GetPayloadSize()*8/inputBitsPerSample;
  PINDEX outputSize = samples*outputBitsPerSample/8;
  output.SetPayloadSize(outputSize);

  switch (inputBitsPerSample) {
    case 16 :
      switch (outputBitsPerSample) {
        case 16 :
          for (i = 0; i < samples; i++)
            *outputWords++ = (short)ConvertOne(*inputWords++);
          break;

        case 8 :
          for (i = 0; i < samples; i++)
            *outputBytes++ = (BYTE)ConvertOne(*inputWords++);
          break;

        case 4 :
          for (i = 0; i < samples; i++) {
            if ((i&1) == 0)
              *outputBytes = (BYTE)ConvertOne(*inputWords++);
            else
              *outputBytes++ |= (BYTE)(ConvertOne(*inputWords++) << 4);
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return FALSE;
      }
      break;

    case 8 :
      switch (outputBitsPerSample) {
        case 16 :
          for (i = 0; i < samples; i++)
            *outputWords++ = (short)ConvertOne(*inputBytes++);
          break;

        case 8 :
          for (i = 0; i < samples; i++)
            *outputBytes++ = (BYTE)ConvertOne(*inputBytes++);
          break;

        case 4 :
          for (i = 0; i < samples; i++) {
            if ((i&1) == 0)
              *outputBytes = (BYTE)ConvertOne(*inputBytes++);
            else
              *outputBytes++ |= (BYTE)(ConvertOne(*inputBytes++) << 4);
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return FALSE;
      }
      break;

    case 4 :
      switch (outputBitsPerSample) {
        case 16 :
          for (i = 0; i < samples; i++)
            if ((i&1) == 0)
              *outputWords++ = (short)ConvertOne(*inputBytes & 15);
            else
              *outputWords++ = (short)ConvertOne(*inputBytes++ >> 4);
          break;

        case 8 :
          for (i = 0; i < samples; i++)
            if ((i&1) == 0)
              *outputBytes++ = (BYTE)ConvertOne(*inputBytes & 15);
            else
              *outputBytes++ = (BYTE)ConvertOne(*inputBytes++ >> 4);
          break;

        case 4 :
          for (i = 0; i < samples; i++) {
            if ((i&1) == 0)
              *outputBytes = (BYTE)ConvertOne(*inputBytes & 15);
            else
              *outputBytes++ |= (BYTE)(ConvertOne(*inputBytes++ >> 4) << 4);
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return FALSE;
      }
      break;

    default :
      PAssertAlways("Unsupported bit size");
      return FALSE;
  }

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

Opal_Linear16Mono_PCM::Opal_Linear16Mono_PCM()
  : OpalStreamedTranscoder(OpalL16_MONO_8KHZ, OpalPCM16, 16, 16, 320)
{
}

int Opal_Linear16Mono_PCM::ConvertOne(int sample) const
{
#if PBYTE_ORDER==PLITTLE_ENDIAN
  return (sample>>8)|(sample<<8);
#else
  return sample;
#endif
}


/////////////////////////////////////////////////////////////////////////////

Opal_PCM_Linear16Mono::Opal_PCM_Linear16Mono()
  : OpalStreamedTranscoder(OpalPCM16, OpalL16_MONO_8KHZ, 16, 16, 320)
{
}


int Opal_PCM_Linear16Mono::ConvertOne(int sample) const
{
#if PBYTE_ORDER==PLITTLE_ENDIAN
  return (sample>>8)|(sample<<8);
#else
  return sample;
#endif
}


/////////////////////////////////////////////////////////////////////////////
