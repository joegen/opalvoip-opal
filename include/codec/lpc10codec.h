/*
 * lpc10codec.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2001 Equivalence Pty. Ltd.
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
 * $Log: lpc10codec.h,v $
 * Revision 1.2009  2005/02/21 12:19:45  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.7  2004/09/01 12:21:27  rjongbloed
 * Added initialisation of H323EndPoints capability table to be all codecs so can
 *   correctly build remote caps from fqast connect params. This had knock on effect
 *   with const keywords added in numerous places.
 *
 * Revision 2.6  2002/11/10 23:22:17  robertj
 * Cosmetic change
 *
 * Revision 2.5  2002/11/10 11:33:16  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.4  2002/09/16 02:52:33  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.3  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.2  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.1  2001/08/01 05:03:09  robertj
 * Changes to allow control of linking software transcoders, use macros
 *   to force linking.
 * Allowed codecs to be used without H.,323 being linked by using the
 *   new NO_H323 define.
 * Major changes to H.323 capabilities, uses OpalMediaFormat for base name.
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.9  2002/09/16 01:14:15  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 1.8  2002/09/03 05:41:25  robertj
 * Normalised the multi-include header prevention ifdef/define symbol.
 * Added globally accessible functions for media format name.
 *
 * Revision 1.7  2002/08/14 19:35:08  rogerh
 * fix typo
 *
 * Revision 1.6  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.5  2001/10/24 01:20:34  robertj
 * Added code to help with static linking of H323Capability names database.
 *
 * Revision 1.4  2001/02/09 05:16:24  robertj
 * Added #pragma interface for GNU C++.
 *
 * Revision 1.3  2001/01/25 07:27:14  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.2  2000/06/10 09:04:56  rogerh
 * fix typo in a comment
 *
 * Revision 1.1  2000/06/05 04:45:02  robertj
 * Added LPC-10 2400bps codec
 *
 */

#ifndef __OPAL_LPC10CODEC_H
#define __OPAL_LPC10CODEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/transcoders.h>

#ifndef NO_H323
#include <h323/h323caps.h>
#endif


struct lpc10_encoder_state;
struct lpc10_decoder_state;

#define OPAL_LPC10 "LPC-10"


///////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

/**This class describes the LPC-10 (FS-1015) codec capability.
 */
class H323_LPC10Capability : public H323NonStandardAudioCapability
{
  PCLASSINFO(H323_LPC10Capability, H323NonStandardAudioCapability);

  public:
  /**@name Construction */
  //@{
    /**Create a new LPC-10 capability.
     */
    H323_LPC10Capability(
      const H323EndPoint & endpoint   // Endpoint to get NonStandardInfo from.
    );
  //@}

  /**@name Overrides from class PObject */
  //@{
    /**Create a copy of the object.
      */
    virtual PObject * Clone() const;
  //@}

  /**@name Identification functions */
  //@{
    /**Get the name of the media data format this class represents.
     */
    virtual PString GetFormatName() const;
  //@}
};


#ifdef H323_STATIC_LIB
H323_STATIC_LOAD_REGISTER_CAPABILITY(H323_LPC10Capability);
#endif


#define OPAL_REGISTER_LPC10_H323 \
          H323_REGISTER_CAPABILITY_EP(H323_LPC10Capability, OPAL_LPC10)


#else // ifndef NO_H323

#define OPAL_REGISTER_LPC10_H323

#endif // ifndef NO_H323


///////////////////////////////////////////////////////////////////////////////

class Opal_LPC10_PCM : public OpalFramedTranscoder {
  public:
    Opal_LPC10_PCM(const OpalTranscoderRegistration & registration);
    ~Opal_LPC10_PCM();
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
  protected:
    struct lpc10_decoder_state * decoder;
};


class Opal_PCM_LPC10 : public OpalFramedTranscoder {
  public:
    Opal_PCM_LPC10(const OpalTranscoderRegistration & registration);
    ~Opal_PCM_LPC10();
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
  protected:
    struct lpc10_encoder_state * encoder;
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_LPC10() \
          OPAL_REGISTER_LPC10_H323 \
          OPAL_REGISTER_TRANSCODER(Opal_LPC10_PCM, OPAL_LPC10, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_LPC10, OPAL_PCM16, OPAL_LPC10)


#endif // __OPAL_LPC10CODEC_H


/////////////////////////////////////////////////////////////////////////////
