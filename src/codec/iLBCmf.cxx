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


#define new PNEW


/////////////////////////////////////////////////////////////////////////////


const OpalMediaFormat & GetOpaliLBC()
{
  static char PreferredMode[] = "Preferred Mode";
  class OpaliLBCFormat : public OpalAudioFormatInternal
  {
    public:
      OpaliLBCFormat()
        : OpalAudioFormatInternal(OPAL_iLBC, RTP_DataFrame::DynamicBase, "iLBC",  50, 160, 1, 1, 1, 8000, 0)
      {
        OpalMediaOption * option = new OpalMediaOptionInteger(PreferredMode, false, OpalMediaOption::MaxMerge, 7);
#if OPAL_SIP
        option->SetFMTPName("mode");
        option->SetFMTPDefault("0");
#endif
#if OPAL_H323
        OpalMediaOption::H245GenericInfo info;
        info.ordinal = 1;
        info.mode = OpalMediaOption::H245GenericInfo::Collapsing;
        option->SetH245Generic(info);
#endif
        AddOption(option);

#if OPAL_H323
        option = FindOption(OpalAudioFormat::RxFramesPerPacketOption());
        if (option != NULL) {
          info.ordinal = 0; // All other fields the same as for the mode
          option->SetH245Generic(info);
        }
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
  static OpalMediaFormat const iLBC(new OpaliLBCFormat);
  return iLBC;
}


// End of File ///////////////////////////////////////////////////////////////
