/*
 * g726codec.h
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
 * $Log: g726codec.h,v $
 * Revision 1.2007  2004/05/15 12:53:40  rjongbloed
 * Fixed incorrect laoding of H.323 capability for G.726
 *
 * Revision 2.5  2002/11/10 23:21:49  robertj
 * Cosmetic change
 *
 * Revision 2.4  2002/11/10 11:33:16  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.3  2002/09/16 02:52:33  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.2  2002/09/04 06:01:46  robertj
 * Updated to OpenH323 v1.9.6
 *
 */

#ifndef __OPAL_G726CODEC_H
#define __OPAL_G726CODEC_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <opal/transcoders.h>

#ifndef NO_H323
#include <h323/h323caps.h>
#endif


struct g726_state_s;

#define OPAL_G726_40 "G.726-40k"
#define OPAL_G726_32 "G.726-32k"
#define OPAL_G726_24 "G.726-24k"
#define OPAL_G726_16 "G.726-16k"

extern OpalMediaFormat const OpalG726_40;
extern OpalMediaFormat const OpalG726_32;
extern OpalMediaFormat const OpalG726_24;
extern OpalMediaFormat const OpalG726_16;


///////////////////////////////////////////////////////////////////////////////

#ifndef NO_H323

/**This class describes the G726 codec capability.
 */
class H323_G726_Capability : public H323NonStandardAudioCapability
{
  PCLASSINFO(H323_G726_Capability, H323NonStandardAudioCapability)
	

  public:
    enum Speeds {
      e_40k,
      e_32k,
      e_24k,
      e_16k,
      NumSpeeds
    };

  /**@name Construction */
  //@{
    /**Create a new G.726 capability.
     */
    H323_G726_Capability(
      H323EndPoint & endpoint,  /// Endpoint to get NonStandardInfo from.
      Speeds speed              /// Speed of encoding
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

  /**@name Protocol manipulation */
  //@{
    /**This function is called whenever and outgoing TerminalCapabilitySet
       or OpenLogicalChannel PDU is being constructed for the control channel.
       It allows the capability to set the PDU fields from information in
       members specific to the class.

       The default behaviour sets the data rate field in the PDU.
     */
    virtual BOOL OnSendingPDU(
      H245_AudioCapability & pdu,  /// PDU to set information on
      unsigned packetSize          /// Packet size to use in capability
    ) const;

    /**This function is called whenever and incoming TerminalCapabilitySet
       or OpenLogicalChannel PDU has been used to construct the control
       channel. It allows the capability to set from the PDU fields,
       information in members specific to the class.

       The default behaviour gets the data rate field from the PDU.
     */
    virtual BOOL OnReceivedPDU(
      const H245_AudioCapability & pdu,  /// PDU to get information from
      unsigned & packetSize              /// Packet size to use in capability
    );
  //@}

  protected:
    Speeds speed;
};


#if defined(H323_STATIC_LIB)
H323_STATIC_LOAD_REGISTER_CAPABILITY(H323_G726_40_Capability);
H323_STATIC_LOAD_REGISTER_CAPABILITY(H323_G726_32_Capability);
H323_STATIC_LOAD_REGISTER_CAPABILITY(H323_G726_24_Capability);
H323_STATIC_LOAD_REGISTER_CAPABILITY(H323_G726_16_Capability);
#endif


#define OPAL_REGISTER_G726_H323 \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_G726_40_Capability, OPAL_G726_40, ep) \
    { return new H323_G726_Capability(ep, H323_G726_Capability::e_40k); } \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_G726_32_Capability, OPAL_G726_32, ep) \
    { return new H323_G726_Capability(ep, H323_G726_Capability::e_32k); } \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_G726_24_Capability, OPAL_G726_24, ep) \
    { return new H323_G726_Capability(ep, H323_G726_Capability::e_24k); } \
  H323_REGISTER_CAPABILITY_FUNCTION(H323_G726_16_Capability, OPAL_G726_16, ep) \
    { return new H323_G726_Capability(ep, H323_G726_Capability::e_16k); }

#else // ifndef NO_H323

#define OPAL_REGISTER_G726_H323

#endif // ifndef NO_H323


///////////////////////////////////////////////////////////////////////////////

class Opal_G726_Transcoder : public OpalStreamedTranscoder {
  public:
    Opal_G726_Transcoder(const OpalTranscoderRegistration & registration, unsigned bits);
    ~Opal_G726_Transcoder();
  protected:
    g726_state_s * g726;
};


class Opal_G726_40_PCM : public Opal_G726_Transcoder {
  public:
    Opal_G726_40_PCM(const OpalTranscoderRegistration & registration);
    virtual int ConvertOne(int sample) const;
};


class Opal_PCM_G726_40 : public Opal_G726_Transcoder {
  public:
    Opal_PCM_G726_40(const OpalTranscoderRegistration & registration);
    virtual int ConvertOne(int sample) const;
};


class Opal_G726_32_PCM : public Opal_G726_Transcoder {
  public:
    Opal_G726_32_PCM(const OpalTranscoderRegistration & registration);
    virtual int ConvertOne(int sample) const;
};


class Opal_PCM_G726_32 : public Opal_G726_Transcoder {
  public:
    Opal_PCM_G726_32(const OpalTranscoderRegistration & registration);
    virtual int ConvertOne(int sample) const;
};


class Opal_G726_24_PCM : public Opal_G726_Transcoder {
  public:
    Opal_G726_24_PCM(const OpalTranscoderRegistration & registration);
    virtual int ConvertOne(int sample) const;
};


class Opal_PCM_G726_24 : public Opal_G726_Transcoder {
  public:
    Opal_PCM_G726_24(const OpalTranscoderRegistration & registration);
    virtual int ConvertOne(int sample) const;
};


class Opal_G726_16_PCM : public Opal_G726_Transcoder {
  public:
    Opal_G726_16_PCM(const OpalTranscoderRegistration & registration);
    virtual int ConvertOne(int sample) const;
};


class Opal_PCM_G726_16 : public Opal_G726_Transcoder {
  public:
    Opal_PCM_G726_16(const OpalTranscoderRegistration & registration);
    virtual int ConvertOne(int sample) const;
};


///////////////////////////////////////////////////////////////////////////////

#define OPAL_REGISTER_G726() \
          OPAL_REGISTER_G726_H323 \
          OPAL_REGISTER_TRANSCODER(Opal_G726_40_PCM, OPAL_G726_40, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_G726_40, OPAL_PCM16, OPAL_G726_40); \
          OPAL_REGISTER_TRANSCODER(Opal_G726_32_PCM, OPAL_G726_32, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_G726_32, OPAL_PCM16, OPAL_G726_32); \
          OPAL_REGISTER_TRANSCODER(Opal_G726_24_PCM, OPAL_G726_24, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_G726_24, OPAL_PCM16, OPAL_G726_24); \
          OPAL_REGISTER_TRANSCODER(Opal_G726_16_PCM, OPAL_G726_16, OPAL_PCM16); \
          OPAL_REGISTER_TRANSCODER(Opal_PCM_G726_16, OPAL_PCM16, OPAL_G726_16)


#endif // __OPAL_G726CODEC_H


/////////////////////////////////////////////////////////////////////////////
