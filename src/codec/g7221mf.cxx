/*
 * g7221mf.cxx
 *
 * GSM-AMR Media Format descriptions
 *
 * Open Phone Abstraction Library
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2008 Vox Lucida
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
 * The Original Code is Open Phone Abstraction Library
 *
 * The Initial Developer of the Original Code is Vox Lucida
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#include "g7221mf_inc.cxx"

#include <opal/mediafmt.h>
#include <codec/opalplugin.h>
#include <h323/h323caps.h>


class OpalG7221Format : public OpalAudioFormatInternal
{
  public:
    OpalG7221Format(const char * formatName, unsigned rate)
      : OpalAudioFormatInternal(formatName,
                                RTP_DataFrame::DynamicBase,
                                G7221EncodingName,
                                rate/400,
                                G7221_SAMPLES_PER_FRAME,
                                1, 1, 1,
                                G7221_SAMPLE_RATE)
    {
      OpalMediaOption * option;

#if OPAL_SIP
      option = new OpalMediaOptionInteger(G7221BitRateOptionName,
                                          true,
                                          OpalMediaOption::EqualMerge,
                                          rate, G7221_24K_BIT_RATE, G7221_48K_BIT_RATE);
      option->SetFMTP(G7221BitRateFMTPName, "0");
      AddOption(option);
#endif

#if OPAL_H323
      option = FindOption(OpalAudioFormat::RxFramesPerPacketOption());
      OPAL_SET_MEDIA_OPTION_H245(option, G7221_H241_RxFramesPerPacket);

      option = new OpalMediaOptionInteger(G7221ExtendedModesOptionName, true, OpalMediaOption::IntersectionMerge, 0x70, 0, 255);
      OPAL_SET_MEDIA_OPTION_H245(option, G7221_H241_ExtendedModes);
      AddOption(option);
#endif
    }


    virtual PObject * Clone() const
    {
      return new OpalG7221Format(*this);
    }
};


#if OPAL_H323
  extern const char G7221_24K_Identifier[] = OpalPluginCodec_Identifer_G7221;
  extern const char G7221_32K_Identifier[] = OpalPluginCodec_Identifer_G7221;
  extern const char G7221_48K_Identifier[] = OpalPluginCodec_Identifer_G7221ext;

  #define CAPABILITY(rate) \
    static H323CapabilityFactory::Worker< \
      H323GenericAudioCapabilityTemplate<G7221_##rate##K_Identifier, GetOpalG7221_##rate##K, G7221_##rate##K_BIT_RATE> \
    > capability(G7221FormatName##rate##K, true)
#else
#define CAPABILITY(rate)
#endif


#define FORMAT(rate) \
  const OpalAudioFormat & GetOpalG7221_##rate##K() \
  { \
  static OpalAudioFormat const format(new OpalG7221Format(G7221FormatName##rate##K, G7221_##rate##K_BIT_RATE)); \
  CAPABILITY(rate); \
  return format; \
}


FORMAT(24);
FORMAT(32);
FORMAT(48);


// End of File ///////////////////////////////////////////////////////////////
