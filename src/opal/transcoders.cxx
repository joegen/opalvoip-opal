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
  maxOutputSize = P_MAX_INDEX; // Just something, usually changed by OpalMediaPatch
  outputIsRTP = inputIsRTP = PFalse;
}


bool OpalTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  PWaitAndSignal mutex(updateMutex);

  if (( input.IsValid() &&  input !=  inputMediaFormat) ||
      (output.IsValid() && output != outputMediaFormat)) {
    PAssertAlways(PInvalidParameter);
    return PFalse;
  }

  if (input.IsValid())
    inputMediaFormat = input;

  if (output.IsValid())
    outputMediaFormat = output;

  return PTrue;
}


PBoolean OpalTranscoder::ExecuteCommand(const OpalMediaCommand & /*command*/)
{
  return PFalse;
}


void OpalTranscoder::SetInstanceID(const BYTE * /*instance*/, unsigned /*instanceLen*/)
{
}


RTP_DataFrame::PayloadTypes OpalTranscoder::GetPayloadType(PBoolean input) const
{
  RTP_DataFrame::PayloadTypes pt = (input ? inputMediaFormat : outputMediaFormat).GetPayloadType();
  if (payloadTypeMap.size() > 0) {
    RTP_DataFrame::PayloadMapType::const_iterator iter = payloadTypeMap.find(pt);
    if (iter != payloadTypeMap.end())
      pt = iter->second;
  }

  return pt;
}


PBoolean OpalTranscoder::ConvertFrames(const RTP_DataFrame & input,
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
  output.front().SetTimestamp(input.GetTimestamp());
  output.front().SetMarker(input.GetMarker());

  // set the output payload type directly from the output media format
  // and the input payload directly from the input media format
  output.front().SetPayloadType(outputMediaFormat.GetPayloadType());
  RTP_DataFrame::PayloadTypes pt = inputMediaFormat.GetPayloadType();

  // map payload using payload map
  if (payloadTypeMap.size() > 0) {

    // map output payload type
    RTP_DataFrame::PayloadMapType::iterator r = payloadTypeMap.find(outputMediaFormat.GetPayloadType());
    if (r != payloadTypeMap.end())
      output.front().SetPayloadType(r->second);

    // map input payload type
    r = payloadTypeMap.find(inputMediaFormat.GetPayloadType());
    if (r != payloadTypeMap.end()) 
      pt = r->second;
  }

  // do not transcode if no match
  if (pt != RTP_DataFrame::MaxPayloadType && pt != input.GetPayloadType() && input.GetPayloadSize() > 0) {
    PTRACE(2, "Opal\tExpected payload type " << pt << ", but received " << input.GetPayloadType() << ". Ignoring packet");
    output.RemoveAll();
    return PTrue;
  }

  return Convert(input, output.front());
}


OpalTranscoder * OpalTranscoder::Create(const OpalMediaFormat & srcFormat,
                                        const OpalMediaFormat & destFormat,
                                                   const BYTE * instance,
                                                       unsigned instanceLen)
{
  // Merge the master options for the codecs. Allows user to set
  // encoder options.
  OpalMediaFormat masterFormat((const char *)destFormat); // Don't use copy ctor!
  OpalMediaFormat dstAdjusted = destFormat;
  if (!dstAdjusted.Merge(masterFormat))
    return NULL;

  OpalTranscoder * transcoder = OpalTranscoderFactory::CreateInstance(OpalTranscoderKey(srcFormat, destFormat));
  if (transcoder != NULL) {
    transcoder->UpdateMediaFormats(srcFormat, dstAdjusted);
    transcoder->SetInstanceID(instance, instanceLen);
  }

  return transcoder;
}


PBoolean OpalTranscoder::SelectFormats(unsigned sessionID,
                                   const OpalMediaFormatList & srcFormats,
                                   const OpalMediaFormatList & dstFormats,
                                   OpalMediaFormat & srcFormat,
                                   OpalMediaFormat & dstFormat)
{
  OpalMediaFormatList::const_iterator s, d;

  // Search through the supported formats to see if can pass data
  // directly from the given format to a possible one with no transcoders.
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    dstFormat = *d;
    if (dstFormat.GetDefaultSessionID() == sessionID) {
      for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
        srcFormat = *s;
        if (srcFormat == dstFormat && dstFormat.Merge(srcFormat) && srcFormat.Merge(dstFormat))
          return true;
      }
    }
  }

  // Search for a single transcoder to get from a to b
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    dstFormat = *d;
    if (dstFormat.GetDefaultSessionID() == sessionID) {
      for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
        srcFormat = *s;
        if (srcFormat.GetDefaultSessionID() == sessionID) {
          OpalTranscoderKey search(srcFormat, dstFormat);
          OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
          for (OpalTranscoderIterator i = availableTranscoders.begin(); i != availableTranscoders.end(); ++i) {
            if (search == *i && dstFormat.Merge(srcFormat) && srcFormat.Merge(dstFormat))
              return true;
          }
        }
      }
    }
  }

  // Last gasp search for a double transcoder to get from a to b
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    dstFormat = *d;
    if (dstFormat.GetDefaultSessionID() == sessionID) {
      for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
        srcFormat = *s;
        if (srcFormat.GetDefaultSessionID() == sessionID) {
          OpalMediaFormat intermediateFormat;
          if (FindIntermediateFormat(srcFormat, dstFormat, intermediateFormat) && dstFormat.Merge(srcFormat) && srcFormat.Merge(dstFormat))
            return true;
        }
      }
    }
  }

  return PFalse;
}


bool OpalTranscoder::FindIntermediateFormat(const OpalMediaFormat & srcFormat,
                                            const OpalMediaFormat & dstFormat,
                                            OpalMediaFormat & intermediateFormat)
{
  intermediateFormat = OpalMediaFormat();

  OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
  for (OpalTranscoderIterator find1 = availableTranscoders.begin(); find1 != availableTranscoders.end(); ++find1) {
    if (find1->first == srcFormat) {
      if (find1->second == dstFormat)
        return true;
      for (OpalTranscoderIterator find2 = availableTranscoders.begin(); find2 != availableTranscoders.end(); ++find2) {
        if (find2->first == find1->second && find2->second == dstFormat) {
          OpalMediaFormat probableFormat = find1->second;
          if (probableFormat.Merge(srcFormat) && probableFormat.Merge(dstFormat)) {
            intermediateFormat = probableFormat;
            return true;
          }
        }
      }
    }
  }

  return PFalse;
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
  for (OpalMediaFormatList::const_iterator f = formats.begin(); f != formats.end(); ++f) {
    OpalMediaFormat format = *f;
    possibleFormats += format;
    OpalMediaFormatList srcFormats = GetSourceFormats(format);
    for (OpalMediaFormatList::iterator s = srcFormats.begin(); s != srcFormats.end(); ++s) {
      OpalMediaFormatList dstFormats = GetDestinationFormats(*s);
      if (dstFormats.GetSize() > 0) {
        possibleFormats += *s;

        for (OpalMediaFormatList::iterator d = dstFormats.begin(); d != dstFormats.end(); ++d) {
          if (d->IsValid())
            possibleFormats += *d;
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


static unsigned GreatestCommonDivisor(unsigned a, unsigned b)
{
  return b == 0 ? a : GreatestCommonDivisor(b, a % b);
}


bool OpalFramedTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  if (!OpalTranscoder::UpdateMediaFormats(input, output))
    return false;

  unsigned framesPerPacket = outputMediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1);
  unsigned inFrameSize = inputMediaFormat.GetFrameSize();
  unsigned outFrameSize = outputMediaFormat.GetFrameSize();
  unsigned inFrameTime = inputMediaFormat.GetFrameTime();
  unsigned outFrameTime = outputMediaFormat.GetFrameTime();
  unsigned leastCommonMultiple = inFrameTime*outFrameTime/GreatestCommonDivisor(inFrameTime, outFrameTime);
  inputBytesPerFrame = leastCommonMultiple/inFrameTime*inFrameSize*framesPerPacket;
  outputBytesPerFrame = leastCommonMultiple/outFrameTime*outFrameSize*framesPerPacket;
  return true;
}


PINDEX OpalFramedTranscoder::GetOptimalDataFrameSize(PBoolean input) const
{
  return input ? inputBytesPerFrame : outputBytesPerFrame;
}


PBoolean OpalFramedTranscoder::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
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
      return PFalse;

    if (!outputIsRTP)
      output.SetPayloadSize(outLen);
    else if (outLen <= RTP_DataFrame::MinHeaderSize)
      output.SetPayloadSize(0);
    else if (outLen <= output.GetHeaderSize())
      output.SetPayloadSize(0);
    else 
      output.SetPayloadSize(outLen - output.GetHeaderSize());

    return PTrue;
  }

  const BYTE * inputPtr = input.GetPayloadPtr();
  PINDEX inputLength = input.GetPayloadSize();

  if (inputLength == 0) {
    output.SetPayloadSize(outputBytesPerFrame);
    return ConvertSilentFrame (output.GetPayloadPtr());
  }

  // set maximum output payload size
  if (!output.SetPayloadSize((inputLength + inputBytesPerFrame - 1)/inputBytesPerFrame*outputBytesPerFrame))
    return PFalse;

  BYTE * outputPtr = output.GetPayloadPtr();

  PINDEX outLen = 0;

  while (inputLength > 0) {

    PINDEX consumed = inputLength; // PMIN(inputBytesPerFrame, inputLength);
    PINDEX created  = output.GetPayloadSize() - outLen;

    if (!ConvertFrame(inputPtr, consumed, outputPtr, created))
      return PFalse;

    if (consumed == 0 && created == 0)
      break;

    outputPtr   += created;
    outLen      += created;
    inputPtr    += consumed;
    inputLength -= consumed;
  }

  // set actual output payload size
  output.SetPayloadSize(outLen);

  return PTrue;
}

PBoolean OpalFramedTranscoder::ConvertFrame(const BYTE * inputPtr, PINDEX & /*consumed*/, BYTE * outputPtr, PINDEX & /*created*/)
{
  return ConvertFrame(inputPtr, outputPtr);
}

PBoolean OpalFramedTranscoder::ConvertFrame(const BYTE * /*inputPtr*/, BYTE * /*outputPtr*/)
{
  return PFalse;
}

PBoolean OpalFramedTranscoder::ConvertSilentFrame(BYTE *dst)
{
  memset(dst, 0, outputBytesPerFrame);
  return PTrue;
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


PINDEX OpalStreamedTranscoder::GetOptimalDataFrameSize(PBoolean input) const
{
  return ((input ? inputBitsPerSample : outputBitsPerSample)+7)/8 * optimalSamples;
}


PBoolean OpalStreamedTranscoder::Convert(const RTP_DataFrame & input,
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
          return PFalse;
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
          return PFalse;
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
          return PFalse;
      }
      break;

    default :
      PAssertAlways("Unsupported bit size");
      return PFalse;
  }

  return PTrue;
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
