/*
 * gsmamrmf.cxx
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
#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <codec/opalplugin.h>
#include <h323/h323caps.h>


#define new PNEW


enum
{
    H241_RxFramesPerPacket = 0 | PluginCodec_H245_Collapsing | PluginCodec_H245_TCS | PluginCodec_H245_OLC,
    H241_InitialMode       = 1 | PluginCodec_H245_NonCollapsing | PluginCodec_H245_ReqMode,
    H241_VAD               = 2 | PluginCodec_H245_Collapsing    | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
};


/////////////////////////////////////////////////////////////////////////////

class OpalGSMAMRFormat : public OpalAudioFormatInternal
{
  public:
    OpalGSMAMRFormat()
      : OpalAudioFormatInternal(OPAL_GSMAMR, RTP_DataFrame::DynamicBase, "AMR",  33, 160, 1, 1, 1, 8000, 0)
    {
      OpalMediaOption * option = new OpalMediaOptionInteger("Initial Mode", false, OpalMediaOption::MinMerge, 7);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "mode", "0");
      OPAL_SET_MEDIA_OPTION_H245(option, H241_InitialMode);
      AddOption(option);

      option = new OpalMediaOptionBoolean("VAD", false, OpalMediaOption::AndMerge, true);
      OPAL_SET_MEDIA_OPTION_H245(option, H241_VAD);
      AddOption(option);

#if OPAL_H323
      if ((option = FindOption(OpalAudioFormat::RxFramesPerPacketOption())) != NULL)
        OPAL_SET_MEDIA_OPTION_H245(option, H241_RxFramesPerPacket);

      AddOption(new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATIONS, true, "RFC3267,RFC4867"));
#endif
    }
};


#if OPAL_H323
extern const char GSM_AMR_Identifier[] = OpalPluginCodec_Identifer_AMR;
#endif


const OpalAudioFormat & GetOpalGSMAMR()
{
  static OpalAudioFormat const GSMAMR_Format(new OpalGSMAMRFormat);

#if OPAL_H323
  static H323CapabilityFactory::Worker<
    H323GenericAudioCapabilityTemplate<GSM_AMR_Identifier, GetOpalGSMAMR>
  > capability(OPAL_GSMAMR, true);
#endif // OPAL_H323

  return GSMAMR_Format;
}


// End of File ///////////////////////////////////////////////////////////////
