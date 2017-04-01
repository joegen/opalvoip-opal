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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transcoders.h"
#endif

#include <opal_config.h>

#include <opal/transcoders.h>


#define new PNEW
#define PTraceModule() "Transcoder"


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
  , maxOutputSize(32768) // Just something, usually changed by OpalMediaPatch
  , m_sessionID(0)
  , outputIsRTP(false)
  , inputIsRTP(false)
  , acceptEmptyPayload(false)
  , acceptOtherPayloads(false)
  , m_inClockRate(inputMediaFormat.GetClockRate())
  , m_outClockRate(outputMediaFormat.GetClockRate())
  , m_lastPayloadType(RTP_DataFrame::IllegalPayloadType)
  , m_consecutivePayloadTypeMismatches(0)
{
}


bool OpalTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  PWaitAndSignal mutex(updateMutex);

  bool ok = inputMediaFormat.Update(input);
  ok = outputMediaFormat.Update(output) && ok; // Avoid McCarthy boolean

  m_inClockRate = inputMediaFormat.GetClockRate();
  m_outClockRate = outputMediaFormat.GetClockRate();

  return ok;
}


PBoolean OpalTranscoder::ExecuteCommand(const OpalMediaCommand & command)
{
  if (outputMediaFormat.IsTransportable()) {
    const OpalMediaFlowControl * flow = dynamic_cast<const OpalMediaFlowControl *>(&command);
    if (flow != NULL) {
      unsigned bitRate = std::min(flow->GetMaxBitRate(), outputMediaFormat.GetMaxBandwidth());
      if ((unsigned)outputMediaFormat.GetOptionInteger(OpalMediaFormat::TargetBitRateOption()) != bitRate) {
        outputMediaFormat.SetOptionInteger(OpalMediaFormat::TargetBitRateOption(), bitRate);
        UpdateMediaFormats(OpalMediaFormat(), outputMediaFormat);
      }
      return true;
    }
  }

  return false;
}


void OpalTranscoder::SetMaxOutputSize(PINDEX size)
{
  maxOutputSize = size;

  if (outputMediaFormat.GetOptionInteger(OpalMediaFormat::MaxTxPacketSizeOption()) > (int)maxOutputSize) {
    PTRACE(4, "Media\tReducing \"" << OpalMediaFormat::MaxTxPacketSizeOption() << "\" to " << maxOutputSize);
    outputMediaFormat.SetOptionInteger(OpalMediaFormat::MaxTxPacketSizeOption(), maxOutputSize);
    UpdateMediaFormats(inputMediaFormat, outputMediaFormat);
  }
}


void OpalTranscoder::NotifyCommand(const OpalMediaCommand & command) const
{
  if (commandNotifier != PNotifier())
    commandNotifier(const_cast<OpalMediaCommand &>(command), m_sessionID);
  else
    PTRACE(4, "No command notifier available for transcoder " << this);
}


void OpalTranscoder::SetInstanceID(const BYTE * /*instance*/, unsigned /*instanceLen*/)
{
}


RTP_DataFrame::PayloadTypes OpalTranscoder::GetPayloadType(PBoolean input) const
{
  PWaitAndSignal mutex(updateMutex);
  return (input ? inputMediaFormat : outputMediaFormat).GetPayloadType();
}


#if OPAL_STATISTICS
void OpalTranscoder::GetStatistics(OpalMediaStatistics & /*statistics*/) const
{
}
#endif


void OpalTranscoder::CopyTimestamp(RTP_DataFrame & dst, const RTP_DataFrame & src, bool inToOut) const
{
  unsigned timestamp = src.GetTimestamp();
  if (m_inClockRate != m_outClockRate) {
    if (inToOut)
      timestamp = (unsigned)((PUInt64)timestamp*m_outClockRate/m_inClockRate);
    else
      timestamp = (unsigned)((PUInt64)timestamp*m_inClockRate/m_outClockRate);
  }
  dst.SetTimestamp(timestamp);
}


PBoolean OpalTranscoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output)
{
  PWaitAndSignal mutex(updateMutex);

  // make sure there is at least one output frame available
  if (output.IsEmpty())
    output.Append(new RTP_DataFrame((PINDEX)0, maxOutputSize));
  else {
    while (output.GetSize() > 1)
      output.RemoveTail();
  }

  RTP_DataFrame & outframe = output.front();
  outframe.SetPayloadSize(0);
  outframe.CopyHeader(input);

  // set the output timestamp and marker bit
  CopyTimestamp(outframe, input, true);

  // set the output payload type directly from the output media format
  // and the input payload directly from the input media format
  outframe.SetPayloadType(GetPayloadType(false));

  // Check for if we handle empty payload packets, if not just return empty payload packet
  if (input.GetPayloadSize() == 0) {
    if (AcceptEmptyPayload())
      return Convert(input, outframe);
    return true;
  }

  RTP_DataFrame::PayloadTypes pt = input.GetPayloadType();

  // Check for if we can handle comfort noise, if not just return empty payload packet
  if (!AcceptComfortNoise() && (pt == RTP_DataFrame::CN || pt == RTP_DataFrame::Cisco_CN)) {
    PTRACE(5, "Removing comfort noise frame with payload type " << pt);
    return true;
  }

  // Check if we can handle different payload types
  if (AcceptOtherPayloads() || pt == GetPayloadType(true))
    return Convert(input, outframe);

  // If not see if we get a lot of consecutive ones
  if (pt != m_lastPayloadType) {
    m_consecutivePayloadTypeMismatches = 0;
    m_lastPayloadType = pt;
  }
  else if (++m_consecutivePayloadTypeMismatches > 10) {
    // OK, we give in, you are really sending this payload type. Hopefully the actual codec is right!
    PTRACE(2, "Consecutive mismatched payload type, expected "  << GetPayloadType(true) << ", now using " << pt);
    inputMediaFormat.SetPayloadType(pt);
    return Convert(input, outframe);
  }

  PTRACE(4, "Removing frame with mismatched payload type " << pt << " - should be " << GetPayloadType(true));
  return true;
}


OpalTranscoder * OpalTranscoder::Create(const OpalMediaFormat & srcFormat,
                                        const OpalMediaFormat & destFormat,
                                                   const BYTE * instance,
                                                       unsigned instanceLen)
{
  OpalTranscoder * transcoder = OpalTranscoderFactory::CreateInstance(OpalTranscoderKey(srcFormat.GetName(), destFormat.GetName()));
  if (transcoder == NULL) {
    PTRACE2(2, NULL, "Could not create transcoder instance from " << srcFormat << " to " << destFormat);
    return NULL;
  }

  transcoder->SetInstanceID(instance, instanceLen); // Make sure this is done first

  /* Set the actual media formats used to create the transcoder.
     This information got lost as OpalTranscoderKey is just two PStrings
     and not OpalMediaFormats. Need to copy all of the actual options
     over the top of the global ones just created. */
  transcoder->inputMediaFormat = srcFormat;
  transcoder->outputMediaFormat = destFormat;

  // This is called to make sure options are sent to plug ins.
  if (transcoder->UpdateMediaFormats(srcFormat, destFormat))
    return transcoder;

  PTRACE2(2, transcoder, "Error creating transcoder instance from " << srcFormat << " to " << destFormat);
  delete transcoder;
  return NULL;
}


static bool MergeFormats(const OpalMediaFormatList & masterFormats,
                         const OpalMediaFormat & srcCapability,
                         const OpalMediaFormat & dstCapability,
                         OpalMediaFormat & srcFormat,
                         OpalMediaFormat & dstFormat)
{
  /* Do the required merges to get final media format.

     We start with the media options from the master list (if present) as a
     starting point. This represents the local users "desired" options. Then
     we merge in the remote users capabilities, determined from the lists passed
     to OpalTranscoder::SelectFormats(). We finally rmerge the two formats so
     common attributes are agreed upon.
  
     Encoder example:
         sourceMediaFormats = YUV420P[QCIF]                        (from PCSSEndPoint)
         sinkMediaFormats   = H.261[QCIF,CIF],H.263[QCIF,CIF,D,E]  (from remotes capabilities)
         masterMediaFormats = H.263[SQCIF,QCIF,CIF,D]              (from local capabilities)

       OpalTranscoder::SelectFormats above will locate the pair of capabilities:
         srcCapability = YUV420P[QCIF]
         dstCapability = H.263[QCIF,CIF,D,E]

       Then we get from the masterMediaFormats:
         srcFormat = YUV420P[QCIF]
         dstFormat = H.263[SQCIF,QCIF,CIF,D]

       Then merging in the respective capability to the format we get:
         srcFormat = YUV420P[QCIF]          <-- No change, srcCapability is identical
         dstFormat = H.263[QCIF,CIF,D]      <-- Drop SQCIF as remote can't do it
                                            <-- Do not add annex E as we can't do it

       Then merging src into dst and dst into src we get:
         srcFormat = YUV420P[QCIF]          <-- No change, dstFormat is superset
         dstFormat = H.263[QCIF,D]          <-- Drop to CIF as YUV420P can't do it
                                            <-- Annex D is left as YUV420P does not have
                                                this option at all.


     Decoder example:
         sourceMediaFormats = H.261[QCIF,CIF],H.263[QCIF,CIF,D,E]  (from remotes capabilities)
         sinkMediaFormats   = YUV420P[QCIF]                        (from PCSSEndPoint)
         masterMediaFormats = H.263[SQCIF,QCIF,CIF,D]              (from local capabilities)

       OpalTranscoder::SelectFormats above will locate the pair of capabilities:
         srcCapability = H.263[QCIF,CIF,D,E]
         dstCapability = YUV420P[QCIF]

       Then we get from the masterMediaFormats:
         dstFormat = H.263[SQCIF,QCIF,CIF,D]
         srcFormat = YUV420P[QCIF]

       Then merging in the respective capability to the format we get:
         srcFormat = H.263[QCIF,CIF,D]      <-- Drop SQCIF as remote can't do it
                                            <-- Do not add annex E as we can't do it
         dstFormat = YUV420P[QCIF]          <-- No change, srcCapability is identical

       Then merging in capabilities we get:
         srcFormat = H.263[QCIF,D]          <-- Drop to CIF as YUV420P can't do it
                                            <-- Annex D is left as YUV420P does not have
                                                this option at all.
         dstFormat = YUV420P[QCIF]          <-- No change, srcFormat is superset
   */

  OpalMediaFormatList::const_iterator masterFormat = masterFormats.FindFormat(srcCapability);
  if (masterFormat == masterFormats.end()) {
    srcFormat = srcCapability;
    PTRACE(5, "Initial source format from capabilities:\n" << setw(-1) << srcFormat);
  }
  else {
    srcFormat = *masterFormat;
    PTRACE(5, "Initial source format from master:\n" << setw(-1) << srcFormat
                            << "Merging with capability\n" << setw(-1) << srcCapability);
    if (!srcFormat.Merge(srcCapability, true)) // Include the PayloadType.
      return false;
  }

  masterFormat = masterFormats.FindFormat(dstCapability);
  if (masterFormat == masterFormats.end()) {
    dstFormat = dstCapability;
    PTRACE(5, "Initial destination format from capabilities:\n" << setw(-1) << dstFormat);
  }
  else {
    dstFormat = *masterFormat;
    PTRACE(5, "Initial destination format from master:\n" << setw(-1) << dstFormat
                                 << "Merging with capability\n" << setw(-1) << dstCapability);

    if (!dstFormat.Merge(dstCapability, true)) // Include the PayloadType.
      return false;
  }

  if (!srcFormat.Merge(dstFormat))
    return false;

  if (!dstFormat.Merge(srcFormat))
    return false;

  return true;
}


bool OpalTranscoder::SelectFormats(const OpalMediaType & mediaType,
                                   const OpalMediaFormatList & srcFormats,
                                   const OpalMediaFormatList & dstFormats,
                                   const OpalMediaFormatList & masterFormats,
                                   OpalMediaFormat & srcFormat,
                                   OpalMediaFormat & dstFormat)
{
  OpalMediaFormatList::const_iterator s, d;

  // Search through the supported formats to see if can pass data
  // directly from the given format to a possible one with no transcoders.
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
      if (*s == *d && s->GetMediaType() == mediaType && MergeFormats(masterFormats, *s, *d, srcFormat, dstFormat))
        return true;
    }
  }

  // Search for a single transcoder to get from a to b
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
      if (s->GetMediaType() == mediaType || d->GetMediaType() == mediaType) {
        OpalTranscoderKey search(s->GetName(), d->GetName());
        OpalTranscoderList availableTranscoders = OpalTranscoderFactory::GetKeyList();
        for (OpalTranscoderIterator i = availableTranscoders.begin(); i != availableTranscoders.end(); ++i) {
          if (search == *i && MergeFormats(masterFormats, *s, *d, srcFormat, dstFormat))
            return true;
        }
      }
    }
  }

  // Last gasp search for a double transcoder to get from a to b
  for (d = dstFormats.begin(); d != dstFormats.end(); ++d) {
    for (s = srcFormats.begin(); s != srcFormats.end(); ++s) {
      if (s->GetMediaType() == mediaType || d->GetMediaType() == mediaType) {
        OpalMediaFormat intermediateFormat;
        if (FindIntermediateFormat(*s, *d, intermediateFormat) &&
            MergeFormats(masterFormats, *s, *d, srcFormat, dstFormat))
          return true;
      }
    }
  }

  return false;
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

  return false;
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

  // Start possible formats with the formats connection can do directly
  for (OpalMediaFormatList::const_iterator f = formats.begin(); f != formats.end(); ++f)
    possibleFormats += *f;

  // Now calculate all of the possible formats via a transcoder
  for (OpalMediaFormatList::const_iterator f = formats.begin(); f != formats.end(); ++f) {
    OpalMediaFormatList srcFormats = GetSourceFormats(*f);
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
                                           const OpalMediaFormat & outputMediaFormat)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
  , m_emptyPayloadState(AwaitingFirstNonEmptyPayload)
  , m_lastEmptyPayloadTimestamp(0)
{
  CalculateSizes();
}


static unsigned GreatestCommonDivisor(unsigned a, unsigned b)
{
  return b == 0 ? a : GreatestCommonDivisor(b, a % b);
}


bool OpalFramedTranscoder::UpdateMediaFormats(const OpalMediaFormat & input, const OpalMediaFormat & output)
{
  if (!OpalTranscoder::UpdateMediaFormats(input, output))
    return false;

  CalculateSizes();
  return true;
}


void OpalFramedTranscoder::CalculateSizes()
{
  unsigned framesPerPacket = outputMediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(),
                              inputMediaFormat.GetOptionInteger(OpalAudioFormat::TxFramesPerPacketOption(), 1));
  // Use num channels from raw side, the network side is sometimes not actually what is used, e.g. Opus.
  unsigned inFrameSize = inputMediaFormat.GetFrameSize();
  unsigned outFrameSize = outputMediaFormat.GetFrameSize();
  unsigned inFrameTime = inputMediaFormat.GetFrameTime()*1000000/m_inClockRate;   // microseconds
  unsigned outFrameTime = outputMediaFormat.GetFrameTime()*1000000/m_outClockRate; // microseconds
  unsigned leastCommonMultiple = inFrameTime*outFrameTime/GreatestCommonDivisor(inFrameTime, outFrameTime);
  inputBytesPerFrame = leastCommonMultiple/inFrameTime*inFrameSize*framesPerPacket;
  outputBytesPerFrame = leastCommonMultiple/outFrameTime*outFrameSize*framesPerPacket;

  unsigned inMaxTimePerFrame  = inFrameTime*inputMediaFormat.GetOptionInteger(OpalAudioFormat::MaxFramesPerPacketOption(), 1);
  unsigned outMaxTimePerFrame = outFrameTime*outputMediaFormat.GetOptionInteger(OpalAudioFormat::MaxFramesPerPacketOption(), 1);
  maxOutputDataSize = outputBytesPerFrame*std::max(inMaxTimePerFrame, outMaxTimePerFrame)/outFrameTime;
}


PINDEX OpalFramedTranscoder::GetOptimalDataFrameSize(PBoolean input) const
{
  return input ? inputBytesPerFrame : outputBytesPerFrame;
}


PBoolean OpalFramedTranscoder::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
{
  // Note updateMutex should already be locked at this point.

  if (inputIsRTP || outputIsRTP) {

    const BYTE * inputPtr;
    PINDEX inLen;
    if (inputIsRTP) {
      inputPtr = (const BYTE *)input;
      inLen = input.GetPacketSize();
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
      return false;

    if (!outputIsRTP)
      output.SetPayloadSize(outLen);
    else if (outLen <= RTP_DataFrame::MinHeaderSize)
      output.SetPayloadSize(0);
    else if (outLen <= output.GetHeaderSize())
      output.SetPayloadSize(0);
    else 
      output.SetPayloadSize(outLen - output.GetHeaderSize());

    return true;
  }

  // set maximum output payload size
  if (!output.SetPayloadSize(maxOutputDataSize))
    return false;

  BYTE * outputPtr = output.GetPayloadPtr();
  PINDEX outLen = 0;

  const BYTE * inputPtr = input.GetPayloadPtr();
  PINDEX inputLength = input.GetPayloadSize();

  if (inputLength == 0) {
    if (AcceptEmptyPayload()) {
      outLen = maxOutputDataSize;
      if (!ConvertFrame(inputPtr, inputLength, outputPtr, outLen))
        return false;

      /* If the codec is delaying the frame data (e.g. due to FEC of Opus) then
         remember the timestamp of the reconstructed frame. */
      if (outLen == 0 && m_emptyPayloadState == AwaitingEmptyPayload) {
        m_emptyPayloadState = DelayedDecodeEmptyPayload;
        m_lastEmptyPayloadTimestamp = input.GetTimestamp();
        PTRACE(4, "Start of delayed frame output on empty payload: ts=" << m_lastEmptyPayloadTimestamp);
      }
    }
    else {
      if (!ConvertSilentFrame(outputPtr))
        return false;
      outLen = outputBytesPerFrame;
    }
  }
  else {
    while (inputLength > 0 && outLen < maxOutputDataSize) {

      PINDEX consumed = inputLength;
      PINDEX created = maxOutputDataSize - outLen;

      if (!ConvertFrame(inputPtr, consumed, outputPtr, created))
        return false;

      // If did not consume or produce any data, codec has gone wrong, abort!
      if (consumed == 0 && created == 0)
        break;

      outputPtr += created;
      outLen += created;
      inputPtr += consumed;
      inputLength -= consumed;
    }
  }

  // We have delayed output from codec, so use timestamp from original sample
  if (outLen > 0) {
    if (m_emptyPayloadState == DelayedDecodeEmptyPayload) {
      PTRACE(4, "Using delayed frame output on silence: ts=" << m_lastEmptyPayloadTimestamp);
      output.SetTimestamp(m_lastEmptyPayloadTimestamp);
    }
    m_emptyPayloadState = AwaitingEmptyPayload;
  }

  // set actual output payload size
  output.SetPayloadSize(outLen);

  return true;
}

PBoolean OpalFramedTranscoder::ConvertFrame(const BYTE * inputPtr, PINDEX & /*consumed*/, BYTE * outputPtr, PINDEX & /*created*/)
{
  return ConvertFrame(inputPtr, outputPtr);
}

PBoolean OpalFramedTranscoder::ConvertFrame(const BYTE * /*inputPtr*/, BYTE * /*outputPtr*/)
{
  return false;
}

PBoolean OpalFramedTranscoder::ConvertSilentFrame(BYTE *dst)
{
  memset(dst, 0, outputBytesPerFrame);
  return true;
}

/////////////////////////////////////////////////////////////////////////////

OpalStreamedTranscoder::OpalStreamedTranscoder(const OpalMediaFormat & inputMediaFormat,
                                               const OpalMediaFormat & outputMediaFormat,
                                               unsigned inputBits,
                                               unsigned outputBits)
  : OpalTranscoder(inputMediaFormat, outputMediaFormat)
{
  inputBitsPerSample = inputBits;
  outputBitsPerSample = outputBits;
}


PINDEX OpalStreamedTranscoder::GetOptimalDataFrameSize(PBoolean input) const
{
  // For streamed codecs a "frame" is one milliseconds worth of data
  PString framesPerPacketOption = input ? OpalAudioFormat::TxFramesPerPacketOption()
                                        : OpalAudioFormat::RxFramesPerPacketOption();
  PINDEX size = outputMediaFormat.GetOptionInteger(framesPerPacketOption,
                 inputMediaFormat.GetOptionInteger(framesPerPacketOption, 1));

  size *= outputMediaFormat.GetClockRate()/1000;            // Convert to milliseconds
  size *= input ? inputBitsPerSample : outputBitsPerSample; // Total bits
  size = (size+7)/8;                                        // Total bytes

  return size > 0 ? size : 1;
}


PBoolean OpalStreamedTranscoder::Convert(const RTP_DataFrame & input,
                                     RTP_DataFrame & output)
{
  PINDEX i, bit, mask;

  PINDEX samples = input.GetPayloadSize()*8/inputBitsPerSample;
  PINDEX outputSize = (samples * outputBitsPerSample + 7) / 8;
  output.SetPayloadSize(outputSize);

  // The conversion algorithm for 5,3 & 2 bits per sample needs an extra
  // couple of bytes at the end of the buffer to avoid lots of conditionals
  output.SetMinSize(output.GetHeaderSize()+outputSize+2);

  const BYTE * inputBytes = input.GetPayloadPtr();
  const short * inputWords = (const short *)inputBytes;

  BYTE * outputBytes = output.GetPayloadPtr();
  short * outputWords = (short *)outputBytes;

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

        case 5 :
        case 3 :
        case 2 :
          bit = 0;
          outputBytes[0] = 0;
          for (i = 0; i < samples; i++) {
            int converted = ConvertOne(*inputWords++);
            outputBytes[0] |= (BYTE)(converted << bit);
            outputBytes[1] |= (BYTE)(converted >> (8-bit));
            bit += outputBitsPerSample;
            if (bit >= 8) {
              outputBytes++;
              outputBytes[1] = 0;
              bit -= 8;
            }
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return false;
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

        case 5 :
        case 3 :
        case 2 :
          bit = 0;
          outputBytes[0] = 0;
          for (i = 0; i < samples; i++) {
            int converted = ConvertOne(*inputBytes++);
            outputBytes[0] |= (BYTE)(converted << bit);
            outputBytes[1] |= (BYTE)(converted >> (8-bit));
            bit += outputBitsPerSample;
            if (bit >= 8) {
              outputBytes++;
              outputBytes[1] = 0;
              bit -= 8;
            }
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return false;
      }
      break;

    case 4 :
      switch (outputBitsPerSample) {
        case 16 :
          for (i = 0; i < samples; i++)
            *outputWords++ = (short)ConvertOne((i&1) == 0 ? (*inputBytes & 15) : (*inputBytes++ >> 4));
          break;

        case 8 :
          for (i = 0; i < samples; i++)
            *outputBytes++ = (BYTE)ConvertOne((i&1) == 0 ? (*inputBytes & 15) : (*inputBytes++ >> 4));
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
          return false;
      }
      break;

    case 5 :
    case 3 :
    case 2 :
      switch (outputBitsPerSample) {
        case 16 :
          bit = 0;
          mask = 0xff>>(8-inputBitsPerSample);
          for (i = 0; i < samples; i++) {
            *outputWords++ = (short)ConvertOne(((inputBytes[0]>>bit)|(inputBytes[1]<<(8-bit)))&mask);
            bit += inputBitsPerSample;
            if (bit >= 8) {
              ++inputBytes;
              bit -= 8;
            }
          }
          break;

        case 8 :
          bit = 0;
          mask = 0xff>>(8-inputBitsPerSample);
          for (i = 0; i < samples; i++) {
            *outputBytes++ = (BYTE)ConvertOne(((inputBytes[0]>>bit)|(inputBytes[1]<<(8-bit)))&mask);
            bit += outputBitsPerSample;
            if (bit >= 8) {
              ++inputBytes;
              bit -= 8;
            }
          }
          break;

        default :
          PAssertAlways("Unsupported bit size");
          return false;
      }
      break;

    default :
      PAssertAlways("Unsupported bit size");
      return false;
  }

  return true;
}


/////////////////////////////////////////////////////////////////////////////

Opal_Linear16Mono_PCM::Opal_Linear16Mono_PCM()
  : OpalStreamedTranscoder(OpalL16_MONO_8KHZ, OpalPCM16, 16, 16)
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
  : OpalStreamedTranscoder(OpalPCM16, OpalL16_MONO_8KHZ, 16, 16)
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
