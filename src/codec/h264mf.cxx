/*
 * h264mf.cxx
 *
 * H.264 Media Format descriptions
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

#if PTRACING
  // Tracing macro different in plug ins
  #undef PTRACE
  #define PTRACE(level, module, args) PTRACE2(level, PTraceObjectInstance(), module << '\t' << args)
#endif

#include "h264mf_inc.cxx"

#if PTRACING
  // Now put the tracing  back again
  #undef PTRACE
  #define PTRACE(level, args) PTRACE2(level, PTraceObjectInstance(), args)
#endif

#include <opal/mediafmt.h>
#include <h323/h323caps.h>


class OpalH264Format : public OpalVideoFormatInternal
{
  public:
    enum PacketizationMode {
      Aligned,
      NonInterleaved,
      Interleaved // Unused as yet
    };

    OpalH264Format(const char * formatName, PacketizationMode mode)
      : OpalVideoFormatInternal(formatName, RTP_DataFrame::DynamicBase, H264EncodingName,
                                PVideoFrameInfo::MaxWidth, PVideoFrameInfo::MaxHeight, 30, 16000000)
    {
      OpalMediaOption * option;
#if OPAL_H323
      OpalMediaOption::H245GenericInfo info;
#endif

      static const char * const profiles[] = {
        H264_PROFILE_STR_BASELINE,
        H264_PROFILE_STR_MAIN,
        H264_PROFILE_STR_EXTENDED
      };
      AddOption(option = new OpalMediaOptionEnum(ProfileName, false, profiles, PARRAYSIZE(profiles), OpalMediaOption::MinMerge));

      static const char * const levels[] = {
        H264_LEVEL_STR_1,
        H264_LEVEL_STR_1_b,
        H264_LEVEL_STR_1_1,
        H264_LEVEL_STR_1_2,
        H264_LEVEL_STR_1_3,
        H264_LEVEL_STR_2,
        H264_LEVEL_STR_2_1,
        H264_LEVEL_STR_2_2,
        H264_LEVEL_STR_3,
        H264_LEVEL_STR_3_1,
        H264_LEVEL_STR_3_2,
        H264_LEVEL_STR_4,
        H264_LEVEL_STR_4_1,
        H264_LEVEL_STR_4_2,
        H264_LEVEL_STR_5,
        H264_LEVEL_STR_5_1
      };
      AddOption(option = new OpalMediaOptionEnum(LevelName, false, levels, PARRAYSIZE(levels), OpalMediaOption::MinMerge, PARRAYSIZE(levels)-1));

#if OPAL_H323
      option = new OpalMediaOptionUnsigned(H241ProfilesName, true, OpalMediaOption::IntersectionMerge, DefaultProfileH241, 1, 127);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_PROFILES));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(H241LevelName, true, OpalMediaOption::MinMerge, DefaultLevelH241, 15, 113);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_LEVEL));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxMBPS_H241_Name, true, OpalMediaOption::MinMerge, 0, 0, 1966);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_CustomMaxMBPS, "0"));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxFS_H241_Name, true, OpalMediaOption::MinMerge, 0, 0, 144);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_CustomMaxFS, "0"));
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxBR_H241_Name, true, OpalMediaOption::MinMerge, 0, 0, 9600);
      option->SetH245Generic(OpalMediaOption::H245GenericInfo(H241_CustomMaxBRandCPB, "0"));
      AddOption(option);

      static const char * const PacketizationModes[] = {
        OpalPluginCodec_Identifer_H264_Aligned","
        OpalPluginCodec_Identifer_H264_Truncated, // Some stupid endpoints (e.g. Polycom) use this, one zero short!

        OpalPluginCodec_Identifer_H264_NonInterleaved","
        OpalPluginCodec_Identifer_H264_Aligned","
        OpalPluginCodec_Identifer_H264_Truncated,

        OpalPluginCodec_Identifer_H264_Interleaved","
        OpalPluginCodec_Identifer_H264_NonInterleaved","
        OpalPluginCodec_Identifer_H264_Aligned","
        OpalPluginCodec_Identifer_H264_Truncated,
      };
      AddOption(new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATIONS, false, PacketizationModes[mode]));
#endif

#if OPAL_SIP
      option = new OpalMediaOptionOctets(SDPProfileAndLevelName, true, OpalMediaOption::NoMerge, false);
      option->SetFMTP(SDPProfileAndLevelFMTPName, SDPProfileAndLevelFMTPDflt);
      option->FromString(DefaultSDPProfileAndLevel);
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxMBPS_SDP_Name, true, OpalMediaOption::MinMerge, 0, 0, 983040);
      option->SetFMTP(MaxMBPS_FMTPName, "0");
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxFS_SDP_Name, true, OpalMediaOption::MinMerge, 0, 0, 36864);
      option->SetFMTP(MaxFS_FMTPName, "0");
      AddOption(option);

      option = new OpalMediaOptionUnsigned(MaxBR_SDP_Name, true, OpalMediaOption::MinMerge, 0, 0, 240000);
      option->SetFMTP(MaxBR_FMTPName, "0");
      AddOption(option);

      option = new OpalMediaOptionUnsigned(PacketizationModeName, true, OpalMediaOption::EqualMerge, mode, 0, 2);
      option->SetFMTP(PacketizationFMTPName, "0");
      AddOption(option);
#endif

      option = new OpalMediaOptionUnsigned(MaxNALUSizeName, false, OpalMediaOption::MinMerge, H241_MAX_NALU_SIZE, 256, 240000);
      OPAL_SET_MEDIA_OPTION_H245(option, H241_Max_NAL_unit_size);
      OPAL_SET_MEDIA_OPTION_FMTP(option, MaxNALUSizeFMTPName, STRINGIZE(H241_MAX_NALU_SIZE));
      AddOption(option);
    }


    virtual PObject * Clone() const
    {
      return new OpalH264Format(*this);
    }


    void GetOriginalOptions(PluginCodec_OptionMap & original)
    {
      PWaitAndSignal m1(media_format_mutex);
      for (PINDEX i = 0; i < options.GetSize(); i++)
        original[options[i].GetName()] = options[i].AsString().GetPointer();
    }

    void SetChangedOptions(const PluginCodec_OptionMap & changed)
    {
      for (PluginCodec_OptionMap::const_iterator it = changed.begin(); it != changed.end(); ++it)
        SetOptionValue(it->first, it->second);
    }

    virtual bool ToNormalisedOptions()
    {
      PluginCodec_OptionMap original, changed;
      GetOriginalOptions(original);
      if (!MyToNormalised(original, changed))
        return false;
      SetChangedOptions(changed);
      return OpalVideoFormatInternal::ToNormalisedOptions();
    }

    virtual bool ToCustomisedOptions()
    {
      PluginCodec_OptionMap original, changed;
      GetOriginalOptions(original);
      if (!MyToCustomised(original, changed))
        return false;
      SetChangedOptions(changed);
      return OpalVideoFormatInternal::ToCustomisedOptions();
    }
};


#if OPAL_H323
extern const char H264_Identifer[] = OpalPluginCodec_Identifer_H264_Generic;
#endif



const OpalVideoFormat & GetOpalH264_MODE0()
{
  static OpalVideoFormat const format(new OpalH264Format(H264_Mode0_FormatName, OpalH264Format::Aligned));

#if OPAL_H323
  static H323CapabilityFactory::Worker<
    H323GenericVideoCapabilityTemplate<H264_Identifer, GetOpalH264_MODE0>
  > capability(H264_Mode0_FormatName, true);
#endif // OPAL_H323

  return format;
}


const OpalVideoFormat & GetOpalH264_MODE1()
{
  static OpalVideoFormat const format(new OpalH264Format(H264_Mode1_FormatName, OpalH264Format::NonInterleaved));

#if OPAL_H323
  static H323CapabilityFactory::Worker<
    H323GenericVideoCapabilityTemplate<H264_Identifer, GetOpalH264_MODE1>
  > capability(H264_Mode1_FormatName, true);
#endif // OPAL_H323

  return format;
}


// End of File ///////////////////////////////////////////////////////////////
