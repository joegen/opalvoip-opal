/*
 * g7231mf.cxx
 *
 * G.723.1 Media Format descriptions
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
 */

#include <ptlib.h>
#include <opal_config.h>

#include <codec/opalplugin.h>
#include <opal/mediafmt.h>
#include <h323/h323caps.h>
#include <asn/h245.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

class H323_G7231Capability : public H323AudioCapability
{
  public:
    virtual PObject * Clone() const
    {
      return new H323_G7231Capability(*this);
    }

    virtual unsigned GetSubType() const
    {
      return H245_AudioCapability::e_g7231;
    }

    virtual PString GetFormatName() const
    {
      return OpalG7231_6k3;
    }

    virtual PBoolean OnSendingPDU(H245_AudioCapability & pdu, unsigned packetSize) const
    {
      pdu.SetTag(H245_AudioCapability::e_g7231);
      H245_AudioCapability_g7231 & g7231 = pdu;
      g7231.m_maxAl_sduAudioFrames = packetSize;
      g7231.m_silenceSuppression = GetMediaFormat().GetOptionBoolean(PLUGINCODEC_OPTION_VOICE_ACTIVITY_DETECT);
      return true;
    }

    virtual PBoolean OnReceivedPDU(const H245_AudioCapability & pdu, unsigned & packetSize)
    {
      if (pdu.GetTag() != H245_AudioCapability::e_g7231)
        return false;

      const H245_AudioCapability_g7231 & g7231 = pdu;
      packetSize = g7231.m_maxAl_sduAudioFrames;
      GetWritableMediaFormat().SetOptionBoolean(PLUGINCODEC_OPTION_VOICE_ACTIVITY_DETECT, g7231.m_silenceSuppression);
      return true;
    }
};


static const unsigned char CapabilityName_G7231ar[] = { 'G', '7', '2', '3', '1', 'a', 'r' };

class H323_G7231_Cisco_A_Capability : public H323NonStandardAudioCapability
{
  public:
    H323_G7231_Cisco_A_Capability()
      : H323NonStandardAudioCapability(181,                                // T35 country code
                                       0,                                  // T35 extension code
                                       18,                                 // T35 manufacturer code
                                       CapabilityName_G7231ar,             // data
                                       sizeof(CapabilityName_G7231ar)-1)   // data length (less the 'r')
    {
    }

    virtual PObject * Clone() const
    {
      return new H323_G7231_Cisco_A_Capability(*this);
    }

    virtual PString GetFormatName() const
    {
      return OpalG7231_6k3;
    }
};


class H323_G7231_Cisco_AR_Capability : public H323NonStandardAudioCapability
{
  public:
    H323_G7231_Cisco_AR_Capability()
      : H323NonStandardAudioCapability(181,                                // T35 country code
                                       0,                                  // T35 extension code
                                       18,                                 // T35 manufacturer code
                                       CapabilityName_G7231ar,             // data
                                       sizeof(CapabilityName_G7231ar))     // data length
    {
    }

    virtual PObject * Clone() const
    {
      return new H323_G7231_Cisco_AR_Capability(*this);
    }

    virtual PString GetFormatName() const
    {
      return OpalG7231_6k3;
    }
};


typedef H323_G7231Capability H323_G7231_6k3_Capability;
typedef H323_G7231Capability H323_G7231_5k3_Capability;
typedef H323_G7231Capability H323_G7231A_6k3_Capability;
typedef H323_G7231Capability H323_G7231A_5k3_Capability;

#define CAPABILITY(type) static H323CapabilityFactory::Worker<H323_##type##_Capability> capability(OPAL_##type, true)

#else
#define CAPABILITY(t)
#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

class OpalG7231Format : public OpalAudioFormatInternal
{
  public:
    OpalG7231Format(const char * variant, unsigned rate, bool isAnnexA)
      : OpalAudioFormatInternal(variant, RTP_DataFrame::G7231, "G723", 24, 240,  8,  3, 256,  8000)
    {
      static const char * const yesno[] = { "no", "yes" };
      OpalMediaOption * option = new OpalMediaOptionEnum(PLUGINCODEC_OPTION_VOICE_ACTIVITY_DETECT, true, yesno, 2, OpalMediaOption::AndMerge, isAnnexA);
#if OPAL_SDP
      option->SetFMTPName("annexa");
      option->SetFMTPDefault("yes");
#endif
      AddOption(option);

      // Automatic calculation comes to 6400, but that includes the tiny, tiny header
      // So need to back it down to the official bit rate.
      SetOptionInteger(OpalMediaFormat::MaxBitRateOption(), rate); 
    }


    virtual PObject * Clone() const
    {
      return new OpalG7231Format(*this);
    }
};

#define FORMAT(name, rate, annex) \
  const OpalAudioFormat & GetOpal##name() \
  { \
    static const OpalMediaFormatStatic<OpalAudioFormat> format(new OpalG7231Format(OPAL_##name, rate, annex)); \
    CAPABILITY(name); \
    return format; \
  }

FORMAT(G7231_6k3, 6300, false);
FORMAT(G7231_5k3, 5300, false);
FORMAT(G7231A_6k3, 6300, true);
FORMAT(G7231A_5k3, 5300, true);
FORMAT(G7231_Cisco_A, 6300, true);
FORMAT(G7231_Cisco_AR, 5300, true);


// End of File ///////////////////////////////////////////////////////////////
