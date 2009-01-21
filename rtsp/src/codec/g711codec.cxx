/*
 * g711codec.cxx
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
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
#pragma implementation "g711codec.h"
#endif

#include <opal/buildopts.h>

#include <codec/g711codec.h>

#define new PNEW

extern "C" {
  int ulaw2linear(int u_val);
  int linear2ulaw(int pcm_val);
  int alaw2linear(int u_val);
  int linear2alaw(int pcm_val);
};



///////////////////////////////////////////////////////////////////////////////

Opal_G711_uLaw_PCM::Opal_G711_uLaw_PCM()
  : OpalStreamedTranscoder(OpalG711_ULAW_64K, OpalPCM16, 8, 16)
{
  PTRACE(3, "Codec\tG711-uLaw-64k decoder created");
#if OPAL_G711PLC 
  acceptEmptyPayload = true;
  lastPayloadSize = 0;
#endif
}


int Opal_G711_uLaw_PCM::ConvertOne(int sample) const
{
  return ulaw2linear(sample);
}


int Opal_G711_uLaw_PCM::ConvertSample(int sample)
{
  return ulaw2linear(sample);
}

#if OPAL_G711PLC 
PBoolean Opal_G711_uLaw_PCM::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
{
  PTRACE(8, "uLaw\t" << (void *)input.GetPayloadPtr() << '\t' << input.GetPayloadSize() << '\t'
         << input.GetSequenceNumber() << '\t' << input.GetTimestamp());

  if (input.GetPayloadSize() == 0) {
    if (output.GetPayloadSize() == 0)
      output.SetPayloadSize(lastPayloadSize);
    plc.dofe((short*)output.GetPayloadPtr(), output.GetPayloadSize()/sizeof(short));
    PTRACE(8, "\tDOFE\t" << (void *)input.GetPayloadPtr() << '\t' << output.GetPayloadSize() << '\t'
           << output.GetSequenceNumber() << '\t' << output.GetTimestamp());
    return true;
  }

  bool result = OpalStreamedTranscoder::Convert(input, output);
  plc.addtohistory((short*)output.GetPayloadPtr(), output.GetPayloadSize()/sizeof(short));
  lastPayloadSize = output.GetPayloadSize();
  PTRACE(8, "\tADD\t" << (void *)input.GetPayloadPtr() << '\t' << output.GetPayloadSize() << '\t'
         << output.GetSequenceNumber() << '\t' << output.GetTimestamp() << '\t' << result);
  return result;
}
#endif


///////////////////////////////////////////////////////////////////////////////

Opal_PCM_G711_uLaw::Opal_PCM_G711_uLaw()
  : OpalStreamedTranscoder(OpalPCM16, OpalG711_ULAW_64K, 16, 8)
{
  PTRACE(3, "Codec\tG711-uLaw-64k encoder created");
}


int Opal_PCM_G711_uLaw::ConvertOne(int sample) const
{
  return linear2ulaw(sample);
}


int Opal_PCM_G711_uLaw::ConvertSample(int sample)
{
  return linear2ulaw(sample);
}


///////////////////////////////////////////////////////////////////////////////

Opal_G711_ALaw_PCM::Opal_G711_ALaw_PCM()
  : OpalStreamedTranscoder(OpalG711_ALAW_64K, OpalPCM16, 8, 16)
{
  PTRACE(3, "Codec\tG711-ALaw-64k decoder created");
#if OPAL_G711PLC 
  acceptEmptyPayload = true;
  lastPayloadSize = 0;
#endif
}


int Opal_G711_ALaw_PCM::ConvertOne(int sample) const
{
  return alaw2linear(sample);
}


int Opal_G711_ALaw_PCM::ConvertSample(int sample)
{
  return alaw2linear(sample);
}

#if OPAL_G711PLC 
PBoolean Opal_G711_ALaw_PCM::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
{
  PTRACE(8, "ALaw\t" << (void *)input.GetPayloadPtr() << '\t' << input.GetPayloadSize() << '\t'
         << input.GetSequenceNumber() << '\t' << input.GetTimestamp());

  if (input.GetPayloadSize() == 0) {
    if (output.GetPayloadSize() == 0)
      output.SetPayloadSize(lastPayloadSize);
    plc.dofe((short*)output.GetPayloadPtr(), output.GetPayloadSize()/sizeof(short));
    PTRACE(8, "\tDOFE\t" << (void *)input.GetPayloadPtr() << '\t' << output.GetPayloadSize() << '\t'
           << output.GetSequenceNumber() << '\t' << output.GetTimestamp());
    return true;
  }

  bool result = OpalStreamedTranscoder::Convert(input, output);
  plc.addtohistory((short*)output.GetPayloadPtr(), output.GetPayloadSize()/sizeof(short));
  lastPayloadSize = output.GetPayloadSize();
  PTRACE(8, "\tADD\t" << (void *)input.GetPayloadPtr() << '\t' << output.GetPayloadSize() << '\t'
         << output.GetSequenceNumber() << '\t' << output.GetTimestamp() << '\t' << result);
  return result;
}
#endif


///////////////////////////////////////////////////////////////////////////////

Opal_PCM_G711_ALaw::Opal_PCM_G711_ALaw()
  : OpalStreamedTranscoder(OpalPCM16, OpalG711_ALAW_64K, 16, 8)
{
  PTRACE(3, "Codec\tG711-ALaw-64k encoder created");
}


int Opal_PCM_G711_ALaw::ConvertOne(int sample) const
{
  return linear2alaw(sample);
}


int Opal_PCM_G711_ALaw::ConvertSample(int sample)
{
  return linear2alaw(sample);
}


/////////////////////////////////////////////////////////////////////////////
