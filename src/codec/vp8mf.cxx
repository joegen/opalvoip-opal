/*
 * vp84mf.cxx
 *
 * Google VP8 Media Format descriptions
 *
 * Open Phone Abstraction Library
 *
 * Copyright (c) 2012 Vox Lucida Pty. Ltd.
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
 */

#include <ptlib.h>

#include <opal_config.h>

#if OPAL_VIDEO

#include "vp8mf_inc.cxx"

#include <opal/mediafmt.h>
#include <h323/h323caps.h>


class OpalVP8Format : public OpalVideoFormatInternal
{
  public:
    OpalVP8Format()
      : OpalVideoFormatInternal(VP8FormatName, RTP_DataFrame::DynamicBase, VP8EncodingName,
                                PVideoFrameInfo::MaxWidth, PVideoFrameInfo::MaxHeight, 30, 16000000)
    {
      OpalMediaOption * option = new OpalMediaOptionUnsigned(MaxFrameRateName, true, OpalMediaOption::MinMerge, MAX_FR_SDP, MIN_FR_SDP, MAX_FR_SDP);
      OPAL_SET_MEDIA_OPTION_FMTP(option, MaxFrameRateSDP, STRINGIZE(MAX_FR_SDP));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxFrameSizeName, true, OpalMediaOption::MinMerge, MAX_FS_SDP, MIN_FS_SDP, MAX_FS_SDP);
      OPAL_SET_MEDIA_OPTION_FMTP(option, MaxFrameSizeSDP, STRINGIZE(MAX_FS_SDP));
      AddOption(option);

      option = new OpalMediaOptionBoolean(SpatialResamplingsName, true, OpalMediaOption::AndMerge);
      OPAL_SET_MEDIA_OPTION_FMTP(option, SpatialResamplingsSDP, "0");
      AddOption(option);
    }

    virtual PObject * Clone() const
    {
      return new OpalVP8Format(*this);
    }


    virtual bool ToNormalisedOptions()
    {
      return AdjustByOptionMaps(PTRACE_PARAM("ToNormalised",) MyToNormalised) && OpalVideoFormatInternal::ToNormalisedOptions();
    }

    virtual bool ToCustomisedOptions()
    {
      return AdjustByOptionMaps(PTRACE_PARAM("ToCustomised",) MyToCustomised) && OpalVideoFormatInternal::ToCustomisedOptions();
    }
};


const OpalVideoFormat & GetOpalVP8()
{
  static OpalVideoFormat const plugin(VP8FormatName);
  if (plugin.IsValid())
    return plugin;

  static OpalVideoFormat const format(new OpalVP8Format());

  return format;
}


#endif // OPAL_VIDEO


// End of File ///////////////////////////////////////////////////////////////
