/*
 * g711codec.h
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
 * $Log: g711codec.h,v $
 * Revision 1.2003  2002/03/15 03:07:25  robertj
 * Added static access to internal conversion functions.
 *
 * Revision 2.1  2001/08/01 05:03:26  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 *
 */

#ifndef __CODEC_G711CODEC_H
#define __CODEC_G711CODEC_H

#ifdef __GNUC__
#pragma interface
#endif


#include <opal/transcoders.h>


///////////////////////////////////////////////////////////////////////////////

class Opal_G711_uLaw_PCM : public OpalStreamedTranscoder {
  public:
    Opal_G711_uLaw_PCM(
      const OpalTranscoderRegistration & registration /// Registration for transcoder
    );
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_G711_uLaw : public OpalStreamedTranscoder {
  public:
    Opal_PCM_G711_uLaw(
      const OpalTranscoderRegistration & registration /// Registration for transcoder
    );
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_G711_ALaw_PCM : public OpalStreamedTranscoder {
  public:
    Opal_G711_ALaw_PCM(
      const OpalTranscoderRegistration & registration /// Registration for transcoder
    );
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_G711_ALaw : public OpalStreamedTranscoder {
  public:
    Opal_PCM_G711_ALaw(
      const OpalTranscoderRegistration & registration /// Registration for transcoder
    );
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_G711() \
  OPAL_REGISTER_TRANSCODER(Opal_G711_uLaw_PCM, OPAL_G711_ULAW_64K, OPAL_PCM16); \
  OPAL_REGISTER_TRANSCODER(Opal_PCM_G711_uLaw, OPAL_PCM16, OPAL_G711_ULAW_64K); \
  OPAL_REGISTER_TRANSCODER(Opal_G711_ALaw_PCM, OPAL_G711_ALAW_64K, OPAL_PCM16); \
  OPAL_REGISTER_TRANSCODER(Opal_PCM_G711_ALaw, OPAL_PCM16, OPAL_G711_ALAW_64K)



#endif // __CODEC_G711CODEC_H


/////////////////////////////////////////////////////////////////////////////
