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
 * $Revision$
 * $Author$
 * $Date$
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
  outputIsRTP = inputIsRTP = FALSE;
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


void OpalTranscoder::SetInstanceID(const BYTE * /*instance*/, unsigned /*instanceLen*/)
{
}


RTP_DataFrame::PayloadTypes OpalTranscoder::GetPayloadType(BOOL input) const
{
  RTP_DataFrame::PayloadTypes pt = (input ? inputMediaFormat : outputMediaFormat).GetPayloadType();
  if (payloadTypeMap.size() > 0) {
    RTP_DataFrame::PayloadMapType::const_iterator iter = payloadTypeMap.find(pt);
    if (iter != payloadTypeMap.end())
      pt = iter->second;
  }

  return pt;
}


BOOL OpalTranscoder::ConvertFrames(const RTP_DataFrame & input,
                                   RTP_DataFrameList & output)
{
  // make sure there is at least one output frame available
  if (output.IsEmpty())
    output.Append(new RTP_DataFrame);
  else {
    while (output.GetSize() > 1)
      output.RemoveAt(1);
  }

  // set the output timestamp and marker bit
  output[0].SetTimestamp(input.GetTimestamp());
  output[0].SetMarker(input.GetMarker());

  // set the output payload type directly from the output media format
  // and the input payload directly from the input media format
  output[0].SetPayloadType(outputMediaFormat.GetPayloadType());
  RTP_DataFrame::PayloadTypes pt = inputMediaFormat.GetPayloadType();

  // map payload using payload map
  if (payloadTypeMap.size() > 0) {

    // map output payload type
    RTP_DataFrame::PayloadMapType::iterator r = payloadTypeMap.find(outputMediaFormat.GetPayloadType());
    if (r != payloadTypeMap.end())
      output[0].SetPayloadType(r->second);

    // map input payload type
    r = payloadTypeMap.find(inputMediaFormat.GetPayloadType());
    if (r != payloadTypeMap.end()) 
      pt = r->second;
  }

  // do not transcode if no match
  if (pt != RTP_DataFrame::MaxPayloadType && pt != input.GetPayloadType()) {
    PTRACE(2, "Opal\tExpected payload type " << pt << ", but received " << input.GetPayloadType() << ". Ignoring packet");
    output.RemoveAll();
    return TRUE;
  }

  return Convert(input, output[0]);
}


OpalTranscoder * OpalTranscoder::Create(const OpalMediaFormat & srcFormat,
                                        const OpalMediaFormat & destFormat,
                                                   const BYTE * instance,
                                                       unsigned instanceLen)
{
  OpalTranscoder * transcoder = OpalTranscoderFactory::CreateInstance(OpalTranscoderKey(srcFormat, destFormat));
  if (transcoder != NULL) {
    transcoder->UpdateOutputMediaFormat(destFormat);
    transcoder->SetInstanceID(instance, instanceLen);
  }
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
          return srcFormat.Merge(dstFormat) &&
                 dstFormat.Merge(srcFormat) &&
                 srcFormat.ToNormalisedOptions() &&
                 dstFormat.ToNormalisedOptions();
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
          OpalTranscoderKey search(srcFormat, dstFormat);
          OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
          for (OpalTranscoderIterator i = availableTranscoders.begin(); i != availableTranscoders.end(); ++i) {
            if (search == *i)
              return srcFormat.Merge(i->first) &&
                     dstFormat.Merge(i->second) &&
                     srcFormat.Merge(dstFormat) &&
                     dstFormat.Merge(srcFormat) &&
                     srcFormat.ToNormalisedOptions() &&
                     dstFormat.ToNormalisedOptions();
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
    if (find1->first == srcFormat) {
      for (OpalTranscoderIterator find2 = availableTranscoders.begin(); find2 != availableTranscoders.end(); ++find2) {
        if (find2->first == find1->second && find2->second == dstFormat) {
          intermediateFormat = find1->second;
          intermediateFormat.Merge(find2->first);
          return srcFormat.Merge(find1->first) &&
                 dstFormat.Merge(find2->second) &&
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
    if (find->first == srcFormat)
      list += find->second;
  }

  return list;
}


OpalMediaFormatList OpalTranscoder::GetSourceFormats(const OpalMediaFormat & dstFormat)
{
  OpalMediaFormatList list;

  OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
  for (OpalTranscoderIterator find = availableTranscoders.begin(); find != availableTranscoders.end(); ++find) {
    if (find->second == dstFormat)
      list += find->first;
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
      OpalMediaFormatList dstFormats = GetDestinationFormats(srcFormats[i]);
      if (dstFormats.GetSize() > 0) {
        possibleFormats += srcFormats[i];

        // if using video, check for two step encoders
        if (format.GetDefaultSessionID() == OpalMediaFormat::DefaultVideoSessionID) {
          for (PINDEX j = 0; j < dstFormats.GetSize(); ++j) {
            if (dstFormats[j].IsValid())
              possibleFormats += dstFormats[j];
          }
        }
      }
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
  PINDEX framesPerPacket = outputMediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
  inputBytesPerFrame = inputBytes*framesPerPacket;
  outputBytesPerFrame = outputBytes*framesPerPacket;
}


PINDEX OpalFramedTranscoder::GetOptimalDataFrameSize(BOOL input) const
{
  return input ? inputBytesPerFrame : outputBytesPerFrame;
}


BOOL OpalFramedTranscoder::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
{
  if (inputIsRTP || outputIsRTP) {

    const BYTE * inputPtr;
    PINDEX inLen;
    if (inputIsRTP) {
      inputPtr = (const BYTE *)input;
      inLen    = input.GetHeaderSize() + input.GetPayloadSize(); 
    }
    else
    {
      inputPtr = input.GetPayloadPtr();
      inLen    = input.GetPayloadSize(); 
    }

    BYTE * outputPtr;
    PINDEX outLen;
    output.SetPayloadSize(outputBytesPerFrame);
    if (outputIsRTP) {
      outputPtr = output.GetPointer();
      outLen    = output.GetSize();
    }
    else
    {
      outputPtr = output.GetPayloadPtr();
      outLen    = outputBytesPerFrame;
    }

    if (!ConvertFrame(inputPtr, inLen, outputPtr, outLen))
      return FALSE;

    if (!outputIsRTP)
      output.SetPayloadSize(outLen);
    else if (outLen <= RTP_DataFrame::MinHeaderSize)
      output.SetPayloadSize(0);
    else if (outLen <= output.GetHeaderSize())
      output.SetPayloadSize(0);
    else 
      output.SetPayloadSize(outLen - output.GetHeaderSize());

    return TRUE;
  }

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

    if (consumed == 0 && created == 0)
      break;

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
  unsigned short tmp_sample = (unsigned short)sample;
  
#if PBYTE_ORDER==PLITTLE_ENDIAN
  return (tmp_sample>>8)|(tmp_sample<<8);
#else
  return tmp_sample;
#endif
}


/////////////////////////////////////////////////////////////////////////////

Opal_PCM_Linear16Mono::Opal_PCM_Linear16Mono()
  : OpalStreamedTranscoder(OpalPCM16, OpalL16_MONO_8KHZ, 16, 16, 320)
{
}


int Opal_PCM_Linear16Mono::ConvertOne(int sample) const
{
  unsigned short tmp_sample = (unsigned short)sample;
  
#if PBYTE_ORDER==PLITTLE_ENDIAN
  return (tmp_sample>>8)|(tmp_sample<<8);
#else
  return tmp_sample;
#endif
}


/////////////////////////////////////////////////////////////////////////////
