/*
 * g729codec.cxx
 *
 * H.323 interface for G.729A codec
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: g729codec.cxx,v $
 * Revision 1.2006  2004/02/19 10:47:02  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.4  2003/06/02 04:04:54  rjongbloed
 * Changed to use new autoconf system
 *
 * Revision 2.3  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.2  2002/09/04 06:01:47  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.1  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 1.7  2003/05/05 11:59:25  robertj
 * Changed to use autoconf style selection of options and subsystems.
 *
 * Revision 1.6  2002/11/12 00:07:12  robertj
 * Added check for Voice Age G.729 only being able to do a single instance
 *   of the encoder and decoder. Now fails the second isntance isntead of
 *   interfering with the first one.
 *
 * Revision 1.5  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.4  2002/06/27 03:13:11  robertj
 * Added G.729 capabilitity support even though is really G.729A.
 * Changed compilation under Windows to use environment variables for
 *   determining if Voice Age G.729A library installed.
 *
 * Revision 1.3  2001/09/21 04:37:30  robertj
 * Added missing GetSubType() function.
 *
 * Revision 1.2  2001/09/21 03:57:18  robertj
 * Fixed warning when no voice age library present.
 * Added pragma interface
 *
 * Revision 1.1  2001/09/21 02:54:47  robertj
 * Added new codec framework with no actual implementation.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "g729codec.h"
#endif

#include <codec/g729codec.h>


#if VOICE_AGE_G729A

extern "C" {
#include "va_g729a.h"
};


#if defined(_MSC_VER)

#pragma comment(lib, VOICE_AGE_G729_LIBRARY)

// All of PWLib/OpenH323 use MSVCRT.LIB or MSVCRTD.LIB, but vag729a.lib uses
// libcmt.lib, so we need to tell the linker to ignore it, can't have two
// Run Time libraries!
#pragma comment(linker, "/NODEFAULTLIB:libcmt.lib")

#endif


#if defined(_MSC_VER)

#pragma comment(lib, VOICE_AGE_G729_LIBRARY)

// All of PWLib/OpenH323 use MSVCRT.LIB or MSVCRTD.LIB, but vag729a.lib uses
// libcmt.lib, so we need to tell the linker to ignore it, can't have two
// Run Time libraries!
#pragma comment(linker, "/NODEFAULTLIB:libcmt.lib")

#endif


#define new PNEW


static OpalFramedTranscoder * voiceAgeEncoderInUse = NULL;
static OpalFramedTranscoder * voiceAgeDecoderInUse = NULL;

enum {
  FrameSize = 10,
  FrameSamples = 80
};


/////////////////////////////////////////////////////////////////////////////

Opal_G729_PCM::Opal_G729_PCM(const OpalTranscoderRegistration & registration)
  : OpalFramedTranscoder(registration, FrameSize, FrameSamples*2)
{
  if (voiceAgeDecoderInUse != NULL) {
    PTRACE(1, "Codec\tVoice Age G.729A decoder already in use!");
    return;
  }
  voiceAgeDecoderInUse = this;
  va_g729a_init_decoder();

  PTRACE(1, "Codec\tG.729A decoder created");
}


Opal_G729_PCM::~Opal_G729_PCM()
{
  if (voiceAgeDecoderInUse == this) {
    voiceAgeDecoderInUse = NULL;
    PTRACE(1, "Codec\tG.729A decoder destroyed");
  }
}


BOOL Opal_G729_PCM::ConvertFrame(const BYTE * src, BYTE * dst)
{
  if (voiceAgeDecoderInUse != this)
    return FALSE;

  va_g729a_decoder((unsigned char *)src, (short *)dst, 0);
  return TRUE;
}


Opal_PCM_G729::Opal_PCM_G729(const OpalTranscoderRegistration & registration)
  : OpalFramedTranscoder(registration, FrameSamples*2, FrameSize)
{
  if (voiceAgeEncoderInUse != NULL) {
    PTRACE(1, "Codec\tVoice Age G.729A encoder already in use!");
    return;
  }
  voiceAgeEncoderInUse = this;
  va_g729a_init_encoder();

  PTRACE(1, "Codec\tG.729A encoder created");
}


Opal_PCM_G729::~Opal_PCM_G729()
{
  if (voiceAgeEncoderInUse == this) {
    voiceAgeEncoderInUse = NULL;
    PTRACE(1, "Codec\tG.729A encoder destroyed");
  }
}


BOOL Opal_PCM_G729::ConvertFrame(const BYTE * src, BYTE * dst)
{
  if (voiceAgeEncoderInUse != this)
    return FALSE;

  va_g729a_encoder((short *)src, dst);
  return TRUE;
}


#endif // VOICE_AGE_G729A


/////////////////////////////////////////////////////////////////////////////
