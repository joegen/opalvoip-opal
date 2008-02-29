/*
 * ilbc.h
 *
 * Internet Low Bitrate Codec
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * $Log: ilbccodec.h,v $
 * Revision 1.2003  2004/04/25 09:27:33  rjongbloed
 * Fixed correct H.323 capability definitions for iLBC codec variants
 *
 * Revision 2.1  2004/02/19 10:46:43  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 1.1  2003/06/06 02:19:04  rjongbloed
 * Added iLBC codec
 *
 */

#ifndef __OPAL_ILBC_H
#define __OPAL_ILBC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/transcoders.h>

#ifndef NO_H323
#include <h323/h323caps.h>
#endif


struct iLBC_Enc_Inst_t_;
struct iLBC_Dec_Inst_t_;


#define OPAL_ILBC_13k3 "iLBC-13k3"
#define OPAL_ILBC_15k2 "iLBC-15k2"

extern OpalMediaFormat const OpaliLBC_13k3;
extern OpalMediaFormat const OpaliLBC_15k2;


///////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

/**This class describes the ILBC codec capability.
 */
class H323_iLBC_Capability : public H323NonStandardAudioCapability
{
  PCLASSINFO(H323_iLBC_Capability, H323NonStandardAudioCapability)

  public:
  /**@name Construction */
  //@{
    enum Speed {
      e_13k3,
      e_15k2
    };

    /**Create a new ILBC capability.
     */
    H323_iLBC_Capability(
      H323EndPoint & endpoint,
      Speed speed
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

  private:
    Speed speed;
};


#ifdef H323_STATIC_LIB
H323_STATIC_LOAD_REGISTER_CAPABILITY(H323_ILBC_13k3_Capability);
H323_STATIC_LOAD_REGISTER_CAPABILITY(H323_ILBC_15k2_Capability);
#endif


#define OPAL_REGISTER_iLBC_H323 \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_ILBC_13k3_Capability, OPAL_ILBC_13k3, ep) \
    { return new H323_iLBC_Capability(ep, H323_iLBC_Capability::e_13k3); } \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_ILBC_15k2_Capability, OPAL_ILBC_15k2, ep) \
    { return new H323_iLBC_Capability(ep, H323_iLBC_Capability::e_15k2); }


#else // ifndef NO_H323

#define OPAL_REGISTER_iLBC_H323

#endif // ifndef NO_H323


class Opal_iLBC_Decoder : public OpalFramedTranscoder {
  public:
    Opal_iLBC_Decoder(const OpalTranscoderRegistration & registration, int speed);
    ~Opal_iLBC_Decoder();
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
  protected:
    struct iLBC_Dec_Inst_t_ * decoder; 
};


class Opal_iLBC_13k3_PCM : public Opal_iLBC_Decoder {
  public:
    Opal_iLBC_13k3_PCM(const OpalTranscoderRegistration & registration);
};


class Opal_iLBC_15k2_PCM : public Opal_iLBC_Decoder {
  public:
    Opal_iLBC_15k2_PCM(const OpalTranscoderRegistration & registration);
};


class Opal_iLBC_Encoder : public OpalFramedTranscoder {
  public:
    Opal_iLBC_Encoder(const OpalTranscoderRegistration & registration, int speed);
    ~Opal_iLBC_Encoder();
    virtual BOOL ConvertFrame(const BYTE * src, BYTE * dst);
  protected:
    struct iLBC_Enc_Inst_t_ * encoder; 
};


class Opal_PCM_iLBC_13k3 : public Opal_iLBC_Encoder {
  public:
    Opal_PCM_iLBC_13k3(const OpalTranscoderRegistration & registration);
};


class Opal_PCM_iLBC_15k2 : public Opal_iLBC_Encoder {
  public:
    Opal_PCM_iLBC_15k2(const OpalTranscoderRegistration & registration);
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_iLBC() \
          OPAL_REGISTER_iLBC_H323 \
          OPAL_REGISTER_TRANSCODER(Opal_iLBC_13k3_PCM, OPAL_ILBC_13k3, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_iLBC_13k3, OPAL_PCM16, OPAL_ILBC_13k3); \
          OPAL_REGISTER_TRANSCODER(Opal_iLBC_15k2_PCM, OPAL_ILBC_15k2, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_iLBC_15k2, OPAL_PCM16, OPAL_ILBC_15k2)


#endif // __OPAL_ILBC_H


/////////////////////////////////////////////////////////////////////////////
