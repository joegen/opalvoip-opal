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
 * Revision 1.2006  2005/08/28 07:59:17  rjongbloed
 * Converted OpalTranscoder to use factory, requiring sme changes in making sure
 *   OpalMediaFormat instances are initialised before use.
 *
 * Revision 2.4  2002/09/16 02:52:33  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.3  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.2  2002/03/15 03:07:25  robertj
 * Added static access to internal conversion functions.
 *
 * Revision 2.1  2001/08/01 05:03:26  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 *
 */

#ifndef __OPAL_G711CODEC_H
#define __OPAL_G711CODEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/transcoders.h>


///////////////////////////////////////////////////////////////////////////////

class Opal_G711_uLaw_PCM : public OpalStreamedTranscoder {
  public:
    Opal_G711_uLaw_PCM();
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_G711_uLaw : public OpalStreamedTranscoder {
  public:
    Opal_PCM_G711_uLaw();
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_G711_ALaw_PCM : public OpalStreamedTranscoder {
  public:
    Opal_G711_ALaw_PCM();
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_G711_ALaw : public OpalStreamedTranscoder {
  public:
    Opal_PCM_G711_ALaw();
    virtual int ConvertOne(int sample) const;
    static int ConvertSample(int sample);
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_G711() \
  OPAL_REGISTER_TRANSCODER(Opal_G711_uLaw_PCM, OpalG711_ULAW_64K, OpalPCM16); \
  OPAL_REGISTER_TRANSCODER(Opal_PCM_G711_uLaw, OpalPCM16,         OpalG711_ULAW_64K); \
  OPAL_REGISTER_TRANSCODER(Opal_G711_ALaw_PCM, OpalG711_ALAW_64K, OpalPCM16); \
  OPAL_REGISTER_TRANSCODER(Opal_PCM_G711_ALaw, OpalPCM16,         OpalG711_ALAW_64K)



#endif // __OPAL_G711CODEC_H


/////////////////////////////////////////////////////////////////////////////
