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
 * $Log: g711codec.cxx,v $
 * Revision 1.2003  2002/03/15 03:07:25  robertj
 * Added static access to internal conversion functions.
 *
 * Revision 2.1  2001/08/01 05:04:14  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "g711codec.h"
#endif

#include <codec/g711codec.h>


#define new PNEW


extern "C" {
  short ulaw2linear(unsigned char u_val);
  unsigned char linear2ulaw(short pcm_val);
  short alaw2linear(unsigned char u_val);
  unsigned char linear2alaw(short pcm_val);
};



///////////////////////////////////////////////////////////////////////////////

Opal_G711_uLaw_PCM::Opal_G711_uLaw_PCM(const OpalTranscoderRegistration & registration)
  : OpalStreamedTranscoder(registration, 8, 16, 160)
{
  PTRACE(3, "Codec\tG711-uLaw-64k decoder created");
}


int Opal_G711_uLaw_PCM::ConvertOne(int sample) const
{
  return ulaw2linear((unsigned char)sample);
}


int Opal_G711_uLaw_PCM::ConvertSample(int sample)
{
  return ulaw2linear((unsigned char)sample);
}


///////////////////////////////////////////////////////////////////////////////

Opal_PCM_G711_uLaw::Opal_PCM_G711_uLaw(const OpalTranscoderRegistration & registration)
  : OpalStreamedTranscoder(registration, 16, 8, 160)
{
  PTRACE(3, "Codec\tG711-uLaw-64k encoder created");
}


int Opal_PCM_G711_uLaw::ConvertOne(int sample) const
{
  return linear2ulaw((unsigned short)sample);
}


int Opal_PCM_G711_uLaw::ConvertSample(int sample)
{
  return linear2ulaw((unsigned short)sample);
}


///////////////////////////////////////////////////////////////////////////////

Opal_G711_ALaw_PCM::Opal_G711_ALaw_PCM(const OpalTranscoderRegistration & registration)
  : OpalStreamedTranscoder(registration, 8, 16, 160)
{
  PTRACE(3, "Codec\tG711-ALaw-64k decoder created");
}


int Opal_G711_ALaw_PCM::ConvertOne(int sample) const
{
  return alaw2linear((unsigned char)sample);
}


int Opal_G711_ALaw_PCM::ConvertSample(int sample)
{
  return alaw2linear((unsigned char)sample);
}


///////////////////////////////////////////////////////////////////////////////

Opal_PCM_G711_ALaw::Opal_PCM_G711_ALaw(const OpalTranscoderRegistration & registration)
  : OpalStreamedTranscoder(registration, 16, 8, 160)
{
  PTRACE(3, "Codec\tG711-ALaw-64k encoder created");
}


int Opal_PCM_G711_ALaw::ConvertOne(int sample) const
{
  return linear2alaw((unsigned short)sample);
}


int Opal_PCM_G711_ALaw::ConvertSample(int sample)
{
  return linear2alaw((unsigned short)sample);
}


/////////////////////////////////////////////////////////////////////////////
