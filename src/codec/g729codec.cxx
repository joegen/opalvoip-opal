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
 * Revision 1.2002  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
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
#endif


#define new PNEW


#if VOICE_AGE_G729A
#pragma message("Using Voice Age G.729A Library")
#else
#pragma message("Using dummy G.729")
#endif

enum {
  FrameSize = 10,
  FrameSamples = 80
};


/////////////////////////////////////////////////////////////////////////////

Opal_G729_PCM::Opal_G729_PCM(const OpalTranscoderRegistration & registration)
  : OpalFramedTranscoder(registration, FrameSize, FrameSamples*2)
{
#if VOICE_AGE_G729A
  va_g729a_init_decoder();
#endif

  PTRACE(1, "Codec\tG.729A decoder created");
}


#if VOICE_AGE_G729A
BOOL Opal_G729_PCM::ConvertFrame(const BYTE * src, BYTE * dst)
{
  va_g729a_decoder((unsigned char *)src, (short *)dst, 0);
  return TRUE;
}
#else
BOOL Opal_G729_PCM::ConvertFrame(const BYTE * , BYTE * )
{
  return FALSE;
}
#endif


Opal_PCM_G729::Opal_PCM_G729(const OpalTranscoderRegistration & registration)
  : OpalFramedTranscoder(registration, FrameSamples*2, FrameSize)
{
#if VOICE_AGE_G729A
  va_g729a_init_encoder();
#endif

  PTRACE(1, "Codec\tG.729A encoder created");
}


#if VOICE_AGE_G729A
BOOL Opal_PCM_G729::ConvertFrame(const BYTE * src, BYTE * dst)
{
  va_g729a_encoder((short *)src, dst);
  return TRUE;
}
#else
BOOL Opal_PCM_G729::ConvertFrame(const BYTE * , BYTE * )
{
  return FALSE;
}
#endif


/////////////////////////////////////////////////////////////////////////////
