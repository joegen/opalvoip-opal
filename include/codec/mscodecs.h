/*
 * mscodecs.h
 *
 * Microsoft nonstandard codecs handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * $Log: mscodecs.h,v $
 * Revision 1.2001  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.6  2001/03/08 01:42:20  robertj
 * Cosmetic changes to recently added MS IMA ADPCM codec.
 *
 * Revision 1.5  2001/03/08 00:57:46  craigs
 * Added MS-IMA codec thanks to Liu Hao. Not yet working - do not use
 *
 * Revision 1.4  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.3  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.2  2001/01/09 23:05:22  robertj
 * Fixed inability to have 2 non standard codecs in capability table.
 *
 * Revision 1.1  2000/08/23 14:23:11  craigs
 * Added prototype support for Microsoft GSM codec
 *
 *
 */

#ifndef __CODEC_MSCODECS_H
#define __CODEC_MSCODECS_H

#ifdef __GNUC__
#pragma interface
#endif


#include <codec/gsmcodec.h>


///////////////////////////////////////////////////////////////////////////////

class Opal_MSGSM_PCM : public Opal_GSM {
  public:
    Opal_MSGSM_PCM(const OpalTranscoderRegistration & registration);
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_PCM_MSGSM : public Opal_GSM {
  public:
    Opal_PCM_MSGSM(const OpalTranscoderRegistration & registration);
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

class Opal_MSIMA_PCM : public OpalFramedTranscoder {
  public:
    Opal_MSIMA_PCM(const OpalTranscoderRegistration & registration);
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
};


///////////////////////////////////////////////////////////////////////////////

struct adpcm_state {
  short valprev;        /* Previous output value */
  char  index;          /* Index into stepsize table */
};

class Opal_PCM_MSIMA : public OpalFramedTranscoder {
  public:
    Opal_PCM_MSIMA(const OpalTranscoderRegistration & registration);
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
  protected:
    adpcm_state s_adpcm;
};


///////////////////////////////////////////////////////////////////////////////

class MicrosoftNonStandardAudioCapability : public H323NonStandardAudioCapability
{
  PCLASSINFO(MicrosoftNonStandardAudioCapability, H323NonStandardAudioCapability);

  public:
    MicrosoftNonStandardAudioCapability(
      const BYTE * header,
      PINDEX headerSize,
      PINDEX offset,
      PINDEX len
    );
};


/////////////////////////////////////////////////////////////////////////

class MicrosoftGSMAudioCapability : public MicrosoftNonStandardAudioCapability
{
  PCLASSINFO(MicrosoftGSMAudioCapability, MicrosoftNonStandardAudioCapability);

  public:
    MicrosoftGSMAudioCapability();
    PObject * MicrosoftGSMAudioCapability::Clone() const;
    PString MicrosoftGSMAudioCapability::GetFormatName() const;
};


/////////////////////////////////////////////////////////////////////////

class MicrosoftIMAAudioCapability : public MicrosoftNonStandardAudioCapability
{
  PCLASSINFO(MicrosoftIMAAudioCapability, MicrosoftNonStandardAudioCapability);

  public:
    MicrosoftIMAAudioCapability();
    PObject * MicrosoftIMAAudioCapability::Clone() const;
    PString MicrosoftIMAAudioCapability::GetFormatName() const;
};


#endif // __CODEC_MSCODECS_H


/////////////////////////////////////////////////////////////////////////
