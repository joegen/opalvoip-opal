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
 * Revision 1.2003  2002/02/13 02:30:37  robertj
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


static OpalTranscoderRegistration * RegisteredTranscodersListHead;


/////////////////////////////////////////////////////////////////////////////

OpalTranscoderRegistration::OpalTranscoderRegistration(const char * input,
                                                       const char * output)
  : PCaselessString(input)
{
  *this += '\t';
  *this += output;

  OpalTranscoderRegistration * test = RegisteredTranscodersListHead;
  while (test != NULL) {
    if (*test == *this)
      return;
    test = test->link;
  }

  link = RegisteredTranscodersListHead;
  RegisteredTranscodersListHead = this;
}


PString OpalTranscoderRegistration::GetInputFormat() const
{
  return Left(Find('\t'));
}


PString OpalTranscoderRegistration::GetOutputFormat() const
{
  return Mid(Find('\t')+1);
}


/////////////////////////////////////////////////////////////////////////////

OpalTranscoder::OpalTranscoder(const OpalTranscoderRegistration & reg)
  : registration(reg)
{
}


void OpalTranscoder::PrintOn(ostream & strm) const
{
  strm << registration.GetInputFormat() << "->"
       << registration.GetOutputFormat();
}


BOOL OpalTranscoder::ConvertFrames(const RTP_DataFrame & input,
                                   RTP_DataFrameList & output)
{
  if (output.IsEmpty())
    output.Append(new RTP_DataFrame);
  else {
    while (output.GetSize() > 1)
      output.RemoveAt(1);
  }

  return Convert(input, output[0]);
}


OpalTranscoder * OpalTranscoder::Create(const OpalMediaFormat & srcFormat,
                                        const OpalMediaFormat & destFormat,
                                        void * parameters)
{
  PString name = srcFormat + '\t' + destFormat;

  OpalTranscoderRegistration * find = RegisteredTranscodersListHead;
  while (find != NULL) {
    if (*find == name)
      return find->Create(parameters);
    find = find->link;
  }

  return NULL;
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
  for (s = 0; s < srcFormats.GetSize(); s++) {
    srcFormat = srcFormats[s];
    if (srcFormat.GetDefaultSessionID() == sessionID) {
      for (d = 0; d < dstFormats.GetSize(); d++) {
        dstFormat = dstFormats[d];
        if (srcFormat == dstFormat)
          return TRUE;
      }
    }
  }

  // Search for a single transcoder to get from a to b
  for (s = 0; s < srcFormats.GetSize(); s++) {
    srcFormat = srcFormats[s];
    if (srcFormat.GetDefaultSessionID() == sessionID) {
      for (d = 0; d < dstFormats.GetSize(); d++) {
        dstFormat = dstFormats[d];
        PString searchName = srcFormat + '\t' + dstFormat;
        OpalTranscoderRegistration * find = RegisteredTranscodersListHead;
        while (find != NULL) {
          if (*find == searchName)
            return TRUE;
          find = find->link;
        }
      }
    }
  }

  // Last gasp search for a double transcoder to get from a to b
  for (s = 0; s < srcFormats.GetSize(); s++) {
    srcFormat = srcFormats[s];
    if (srcFormat.GetDefaultSessionID() == sessionID) {
      for (d = 0; d < dstFormats.GetSize(); d++) {
        dstFormat = dstFormats[d];
        OpalMediaFormat intermediateFormat;
        if (FindIntermediateFormat(srcFormat, dstFormat, intermediateFormat))
          return TRUE;
      }
    }
  }

  return FALSE;
}


BOOL OpalTranscoder::FindIntermediateFormat(const OpalMediaFormat & srcFormat,
                                            const OpalMediaFormat & dstFormat,
                                            OpalMediaFormat & intermediateFormat)
{
  intermediateFormat = OpalMediaFormat();

  OpalTranscoderRegistration * find1 = RegisteredTranscodersListHead;
  while (find1 != NULL) {
    if (find1->GetInputFormat() == srcFormat) {
      intermediateFormat = find1->GetOutputFormat();
      PString searchName = intermediateFormat + '\t' + dstFormat;
      OpalTranscoderRegistration * find2 = RegisteredTranscodersListHead;
      while (find2 != NULL) {
        if (*find2 == searchName)
          return TRUE;
        find2 = find2->link;
      }
    }
    find1 = find1->link;
  }

  return FALSE;
}


OpalMediaFormatList OpalTranscoder::GetDestinationFormats(const OpalMediaFormat & srcFormat)
{
  OpalMediaFormatList list;

  OpalTranscoderRegistration * find = RegisteredTranscodersListHead;
  while (find != NULL) {
    if (find->GetInputFormat() == srcFormat)
      list += find->GetOutputFormat();
    find = find->link;
  }

  return list;
}


OpalMediaFormatList OpalTranscoder::GetSourceFormats(const OpalMediaFormat & dstFormat)
{
  OpalMediaFormatList list;

  OpalTranscoderRegistration * find = RegisteredTranscodersListHead;
  while (find != NULL) {
    if (find->GetOutputFormat() == dstFormat)
      list += find->GetInputFormat();
    find = find->link;
  }

  return list;
}


/////////////////////////////////////////////////////////////////////////////

OpalFramedTranscoder::OpalFramedTranscoder(const OpalTranscoderRegistration & registration,
                                           unsigned inputBytes, unsigned outputBytes)
  : OpalTranscoder(registration),
    partialFrame(inputBytes)
{
  inputBytesPerFrame = inputBytes;
  outputBytesPerFrame = outputBytes;
  partialBytes = 0;
}


unsigned OpalFramedTranscoder::GetOptimalDataFrameSize(BOOL input) const
{
  return input ? inputBytesPerFrame : outputBytesPerFrame;
}


BOOL OpalFramedTranscoder::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
{
  const BYTE * inputPtr = input.GetPayloadPtr();
  unsigned inputLength = input.GetPayloadSize();
  output.SetPayloadSize(inputLength/inputBytesPerFrame*outputBytesPerFrame);
  BYTE * outputPtr = output.GetPayloadPtr();

  while (inputLength > 0) {
    // If have enough data and nothing in the reblocking buffer, just send it
    // straight on to the device.
    if (partialBytes == 0 && inputLength >= inputBytesPerFrame) {
      if (!ConvertFrame(inputPtr, outputPtr))
        return FALSE;
      outputPtr += outputBytesPerFrame;
      inputPtr += inputBytesPerFrame;
      inputLength -= inputBytesPerFrame;
    }
    else {
      BYTE * savedFramePtr = partialFrame.GetPointer(inputBytesPerFrame);

      // See if new chunk gives us enough for one frames worth
      if ((partialBytes + inputLength) < inputBytesPerFrame) {
        // Nope, just copy bytes into buffer and return
        memcpy(savedFramePtr + partialBytes, inputPtr, inputLength);
        partialBytes += inputLength;
        return TRUE;
      }

      /* Calculate bytes we want from the passed in buffer to fill a frame by
         subtracting from full frame width the amount we have so far.
       */
      PINDEX left = inputBytesPerFrame - partialBytes;
      memcpy(savedFramePtr + partialBytes, inputPtr, left);
      partialBytes = 0;

      // Convert the saved frame
      if (!ConvertFrame(inputPtr, outputPtr))
        return FALSE;

      inputPtr += left;
      inputLength -= left;
      outputPtr += outputBytesPerFrame;
    }
  }

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

OpalStreamedTranscoder::OpalStreamedTranscoder(const OpalTranscoderRegistration & registration,
                                               unsigned inputBits,
                                               unsigned outputBits,
                                               unsigned optimal)
  : OpalTranscoder(registration)
{
  inputBitsPerSample = inputBits;
  outputBitsPerSample = outputBits;
  optimalSamples = optimal;
}


unsigned OpalStreamedTranscoder::GetOptimalDataFrameSize(BOOL input) const
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

  PINDEX outputSize = input.GetPayloadSize()*outputBitsPerSample/inputBitsPerSample;
  output.SetPayloadSize(outputSize);

  switch (inputBitsPerSample) {
    case 16 :
      switch (outputBitsPerSample) {
        case 16 :
          for (i = 0; i < outputSize; i++)
            *outputWords++ = (short)ConvertOne(*inputWords++);
          break;

        case 8 :
          for (i = 0; i < outputSize; i++)
            *outputBytes++ = (BYTE)ConvertOne(*inputWords++);
          break;

        case 4 :
          for (i = 0; i < outputSize; i++) {
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
          for (i = 0; i < outputSize; i++)
            *outputWords++ = (short)ConvertOne(*inputBytes++);
          break;

        case 8 :
          for (i = 0; i < outputSize; i++)
            *outputBytes++ = (BYTE)ConvertOne(*inputBytes++);
          break;

        case 4 :
          for (i = 0; i < outputSize; i++) {
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
          for (i = 0; i < outputSize; i++)
            if ((i&1) == 0)
              *outputWords++ = (short)ConvertOne(*inputBytes & 15);
            else
              *outputWords++ |= (short)ConvertOne(*inputBytes++ >> 4);
          break;

        case 8 :
          for (i = 0; i < outputSize; i++)
            if ((i&1) == 0)
              *outputBytes++ = (BYTE)ConvertOne(*inputBytes & 15);
            else
              *outputBytes++ |= (BYTE)ConvertOne(*inputBytes++ >> 4);
          break;

        case 4 :
          for (i = 0; i < outputSize; i++) {
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

OpalVideoTranscoder::OpalVideoTranscoder(const OpalTranscoderRegistration & registration)
  : OpalTranscoder(registration)
{
  frameWidth = frameHeight = 0;
}


unsigned OpalVideoTranscoder::GetOptimalDataFrameSize(BOOL) const
{
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
