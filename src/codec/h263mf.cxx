/*
 * h263mf.cxx
 *
 * H.263 Media Format descriptions
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

#include "h263mf_inc.cxx"

#if PTRACING
  // Now put the tracing  back again
  #undef PTRACE
  #define PTRACE(level, args) PTRACE2(level, PTraceObjectInstance(), args)
#endif

#include <opal/mediafmt.h>
#include <codec/opalpluginmgr.h>
#include <asn/h245.h>


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323

class H323H263baseCapability : public H323H263Capability
{
  public:
    H323H263baseCapability()
      : H323H263Capability(H263FormatName)
    {
    }

    virtual PObject * Clone() const
    {
      return new H323H263baseCapability(*this);
    }
};

class H323H263plusCapability : public H323H263Capability
{
  public:
    H323H263plusCapability()
      : H323H263Capability(H263plusFormatName)
    {
    }

    virtual PObject * Clone() const
    {
      return new H323H263plusCapability(*this);
    }
};

#endif // OPAL_H323


class OpalCustomSizeOption : public OpalMediaOptionString
{
    PCLASSINFO(OpalCustomSizeOption, OpalMediaOptionString)
  public:
    OpalCustomSizeOption(
      const char * name,
      bool readOnly
    ) : OpalMediaOptionString(name, readOnly)
    {
    }

    virtual bool Merge(const OpalMediaOption & option)
    {
      char buffer[MAX_H263_CUSTOM_SIZES*20];
      if (!MergeCustomResolution(m_value, option.AsString(), buffer))
        return false;

      m_value = buffer;
      return true;
    }
};


class OpalH263Format : public OpalVideoFormatInternal
{
  public:
    OpalH263Format(const char * formatName, const char * encodingName)
      : OpalVideoFormatInternal(formatName, RTP_DataFrame::DynamicBase, encodingName,
                                PVideoFrameInfo::CIF16Width, PVideoFrameInfo::CIF16Height, 30, H263_BITRATE)
    {
      OpalMediaOption * option;

      option = new OpalMediaOptionInteger(SQCIF_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "SQCIF", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(QCIF_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "QCIF", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(CIF_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "CIF", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(CIF4_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "CIF4", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger(CIF16_MPI, false, OpalMediaOption::MaxMerge, 1, 1, PLUGINCODEC_MPI_DISABLED);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "CIF16", STRINGIZE(PLUGINCODEC_MPI_DISABLED));
      AddOption(option);

      option = new OpalMediaOptionInteger("MaxBR", false, OpalMediaOption::MinMerge, 0, 0, 32767);
      OPAL_SET_MEDIA_OPTION_FMTP(option, "maxbr", "0");
      AddOption(option);

      if (GetName() == H263FormatName)
        AddOption(new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATION, false, RFC2190));
      else {
        option = new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATIONS, false, RFC2429);
        option->SetMerge(OpalMediaOption::IntersectionMerge);
        AddOption(option);

        option = new OpalCustomSizeOption(PLUGINCODEC_CUSTOM_MPI, false);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "CUSTOM", DEFAULT_CUSTOM_MPI);
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_D, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "D", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_F, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "F", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_I, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "I", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_J, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "J", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_K, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "K", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_N, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "N", "0");
        AddOption(option);

        option = new OpalMediaOptionBoolean(H263_ANNEX_T, false, OpalMediaOption::AndMerge);
        OPAL_SET_MEDIA_OPTION_FMTP(option, "T", "0");
        AddOption(option);
      }
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
      if (!ClampSizes(original, changed))
        return false;
      SetChangedOptions(changed);
      return OpalVideoFormatInternal::ToNormalisedOptions();
    }

    virtual bool ToCustomisedOptions()
    {
      PluginCodec_OptionMap original, changed;
      GetOriginalOptions(original);
      if (!ClampSizes(original, changed))
        return false;
      SetChangedOptions(changed);
      return OpalVideoFormatInternal::ToCustomisedOptions();
    }
};


const OpalVideoFormat & GetOpalH263()
{
  static OpalVideoFormat const format(new OpalH263Format(H263FormatName, H263EncodingName));

#if OPAL_H323
  static H323CapabilityFactory::Worker<H323H263baseCapability> capability(H263FormatName, true);
#endif // OPAL_H323

  return format;
}


const OpalVideoFormat & GetOpalH263plus()
{
  static OpalVideoFormat const format(new OpalH263Format(H263plusFormatName, H263plusEncodingName));

#if OPAL_H323
  static H323CapabilityFactory::Worker<H323H263plusCapability> capability(H263plusFormatName, true);
#endif // OPAL_H323

  return format;
}


// End of File ///////////////////////////////////////////////////////////////
