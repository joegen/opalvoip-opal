/*
 * iLBCmf.cxx
 *
 * iLBC Media Format descriptions
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
#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <codec/opalplugin.h>
#include <h323/h323caps.h>
#include <asn/h245.h>


#define new PNEW


enum
{
    H241_RxFramesPerPacket = 0 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    H241_PreferredMode     = 1 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
};


/////////////////////////////////////////////////////////////////////////////

static char PreferredMode[] = "Preferred Mode";

class OpaliLBCFormat : public OpalAudioFormatInternal
{
  public:
    OpaliLBCFormat()
      : OpalAudioFormatInternal(OPAL_iLBC, RTP_DataFrame::DynamicBase, "iLBC",  50, 160, 1, 1, 1, 8000, 0)
    {
      OpalMediaOption * option = new OpalMediaOptionInteger(PreferredMode, false, OpalMediaOption::MaxMerge, 7);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "mode", "0");
      OPAL_SET_MEDIA_OPTION_H245(option, H241_PreferredMode);
      AddOption(option);

#if OPAL_H323
      if ((option = FindOption(OpalAudioFormat::RxFramesPerPacketOption())) != NULL)
        OPAL_SET_MEDIA_OPTION_H245(option, H241_RxFramesPerPacket);
#endif

      FindOption(OpalMediaFormat::FrameTimeOption())->SetMerge(OpalMediaOption::MaxMerge);
    }

    virtual PObject * Clone() const { return new OpaliLBCFormat(*this); }

    virtual bool ToNormalisedOptions()
    {
      int mode = GetOptionInteger(PreferredMode, 20);
      if (mode == 0)
        return true;

      unsigned frameTime = GetOptionInteger(OpalMediaFormat::FrameTimeOption(), 160);

      if (mode < 25) {
        mode = 20;
        frameTime = 160;
      }
      else {
        mode = 30;
        frameTime = 240;
      }

      return SetOptionInteger(PreferredMode, mode) && SetOptionInteger(OpalMediaFormat::FrameTimeOption(), frameTime);
    }

    virtual bool ToCustomisedOptions()
    {
      int mode = GetOptionInteger(PreferredMode, 20);
      unsigned frameTime = GetOptionInteger(OpalMediaFormat::FrameTimeOption(), 160);

      if (frameTime < 200) {
        mode = 20;
        frameTime = 160;
      }
      else {
        mode = 30;
        frameTime = 240;
      }

      return SetOptionInteger(PreferredMode, mode) && SetOptionInteger(OpalMediaFormat::FrameTimeOption(), frameTime);
    }
};


#if OPAL_H323
extern const char iLBC_Identifier[] = OpalPluginCodec_Identifer_iLBC;
#endif // OPAL_H323


const OpalAudioFormat & GetOpaliLBC()
{
  static OpalAudioFormat const iLBC_Format(new OpaliLBCFormat);

#if OPAL_H323
  static H323CapabilityFactory::Worker<
    H323GenericAudioCapabilityTemplate<iLBC_Identifier, GetOpaliLBC>
  > capability(OPAL_iLBC, true);
#endif // OPAL_H323

  return iLBC_Format;
}


// End of File ///////////////////////////////////////////////////////////////
