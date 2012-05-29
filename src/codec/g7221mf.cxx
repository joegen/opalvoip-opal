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
#if OPAL_SIP
      OpalMediaOption * option = new OpalMediaOptionInteger(G7221BitRateOptionName,
                                                            true,
                                                            OpalMediaOption::EqualMerge,
                                                            rate, G7221_24K_BIT_RATE, G7221_32K_BIT_RATE);
      option->SetFMTP(G7221BitRateFMTPName, "0");
      AddOption(option);
#endif
    }


    virtual PObject * Clone() const
    {
      return new OpalG7221Format(*this);
    }
};


#if OPAL_H323
extern const char G7221_Identifier[] = OpalPluginCodec_Identifer_G7221;
#endif


const OpalAudioFormat & GetOpalG7221_24K()
{
  static OpalAudioFormat const format(new OpalG7221Format(G7221FormatName24K, G7221_24K_BIT_RATE));

#if OPAL_H323
  static H323CapabilityFactory::Worker<
    H323GenericAudioCapabilityTemplate<G7221_Identifier, GetOpalG7221_24K, G7221_24K_BIT_RATE>
  > capability(G7221FormatName24K, true);
#endif

  return format;
}


const OpalAudioFormat & GetOpalG7221_32K()
{
  static OpalAudioFormat const format(new OpalG7221Format(G7221FormatName32K, G7221_32K_BIT_RATE));

#if OPAL_H323
  static H323CapabilityFactory::Worker<
    H323GenericAudioCapabilityTemplate<G7221_Identifier, GetOpalG7221_32K, G7221_32K_BIT_RATE>
  > capability(G7221FormatName32K, true);
#endif

  return format;
}


// End of File ///////////////////////////////////////////////////////////////
