/*
 * lpc10codec.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: lpc10codec.cxx,v $
 * Revision 1.2003  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/01 05:04:28  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 * Allowed codecs to be used without H.,323 being linked by using the
 *   new NO_H323 define.
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.7  2001/09/21 02:51:29  robertj
 * Added default session ID to media format description.
 *
 * Revision 1.6  2001/05/14 05:56:28  robertj
 * Added H323 capability registration system so can add capabilities by
 *   string name instead of having to instantiate explicit classes.
 *
 * Revision 1.5  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.4  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.3  2000/07/12 10:25:37  robertj
 * Renamed all codecs so obvious whether software or hardware.
 *
 * Revision 1.2  2000/06/17 04:09:49  craigs
 * Fixed problem with (possibly bogus) underrun errors being reported in debug mode
 *
 * Revision 1.1  2000/06/05 04:45:11  robertj
 * Added LPC-10 2400bps codec
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "lpc10codec.h"
#endif

#include <codec/lpc10codec.h>

extern "C" {
#include "lpc10/lpc10.h"
};


#define new PNEW


enum {
  SamplesPerFrame = 180,    // 22.5 milliseconds
  BitsPerFrame = 54,        // Encoded size
  BytesPerFrame = (BitsPerFrame+7)/8
};

const real SampleValueScale = 32768.0;
const real MaxSampleValue = 32767.0;
const real MinSampleValue = -32767.0;



OpalMediaFormat const OpalLPC10(
  OPAL_LPC10,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::LPC,
  TRUE,  // Needs jitter
  2400,  // bits/sec
  BytesPerFrame,
  SamplesPerFrame,
  OpalMediaFormat::AudioTimeUnits
);


///////////////////////////////////////////////////////////////////////////////

Opal_LPC10_PCM::Opal_LPC10_PCM(const OpalTranscoderRegistration & registration)
  : OpalFramedTranscoder(registration, BytesPerFrame, SamplesPerFrame*2)
{
  decoder = (struct lpc10_decoder_state *)malloc((unsigned)sizeof(struct lpc10_decoder_state));
  ::init_lpc10_decoder_state(decoder);
  PTRACE(3, "Codec\tLPC-10 decoder created");
}


Opal_LPC10_PCM::~Opal_LPC10_PCM()
{
  if (decoder != NULL)
    free(decoder);
}
 

BOOL Opal_LPC10_PCM::ConvertFrame(const BYTE * src, BYTE * dst)
{
  PINDEX i;

  INT32 bits[BitsPerFrame];
  for (i = 0; i < BitsPerFrame; i++)
    bits[i] = (src[i>>3]&(1<<(i&7))) != 0;

  real speech[SamplesPerFrame];
  lpc10_decode(bits, speech, decoder);

  for (i = 0; i < SamplesPerFrame; i++) {
    real sample = speech[i]*SampleValueScale;
    if (sample < MinSampleValue)
      sample = MinSampleValue;
    else if (sample > MaxSampleValue)
      sample = MaxSampleValue;
    ((short *)dst)[i] = (short)sample;
  }

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

Opal_PCM_LPC10::Opal_PCM_LPC10(const OpalTranscoderRegistration & registration)
  : OpalFramedTranscoder(registration, SamplesPerFrame*2, BytesPerFrame)
{
  encoder = (struct lpc10_encoder_state *)malloc((unsigned)sizeof(struct lpc10_encoder_state));
  ::init_lpc10_encoder_state(encoder);
  PTRACE(3, "Codec\tLPC-10 encoder created");
}


Opal_PCM_LPC10::~Opal_PCM_LPC10()
{
  if (encoder != NULL)
    free(encoder);
}
 

BOOL Opal_PCM_LPC10::ConvertFrame(const BYTE * src, BYTE * dst)
{
  PINDEX i;

  real speech[SamplesPerFrame];
  for (i = 0; i < SamplesPerFrame; i++)
    speech[i] = ((short *)src)[i]/SampleValueScale;

  INT32 bits[BitsPerFrame];
  lpc10_encode(speech, bits, encoder);

  memset(dst, 0, BytesPerFrame);
  for (i = 0; i < BitsPerFrame; i++) {
    if (bits[i])
      dst[i>>3] |= 1 << (i&7);
  }

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////

H323_LPC10Capability::H323_LPC10Capability(H323EndPoint & endpoint)
  : H323NonStandardAudioCapability(OpalLPC10, 7, 4, endpoint)
{
}


PObject * H323_LPC10Capability::Clone() const
{
  return new H323_LPC10Capability(*this);
}


/////////////////////////////////////////////////////////////////////////////
