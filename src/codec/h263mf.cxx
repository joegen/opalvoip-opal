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
 */

#include <ptlib.h>

#include <opal_config.h>

#if OPAL_VIDEO

#include "h263mf_inc.cxx"

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
      char buffer[H263_CUSTOM_RESOLUTION_BUFFER_SIZE];
      if (!MergeCustomResolution(m_value, option.AsString(), buffer))
        return false;

      m_value = buffer;
      return true;
    }
};


class OpalH263FormatInternal : public OpalVideoFormatInternal
{
  public:
    OpalH263FormatInternal(const char * formatName, const char * encodingName, RTP_DataFrame::PayloadTypes payloadType)
      : OpalVideoFormatInternal(formatName, payloadType, encodingName,
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


    virtual PObject * Clone() const
    {
      return new OpalH263FormatInternal(*this);
    }


    virtual bool ToNormalisedOptions()
    {
      return AdjustByOptionMaps(PTRACE_PARAM("ToNormalised",) ClampToNormalised) && OpalVideoFormatInternal::ToNormalisedOptions();
    }

    virtual bool ToCustomisedOptions()
    {
      return AdjustByOptionMaps(PTRACE_PARAM("ToCustomised",) ClampToCustomised) && OpalVideoFormatInternal::ToCustomisedOptions();
    }
};


#if OPAL_H323
typedef OpalMediaFormatStaticH323<OpalVideoFormat, H323H263baseCapability> OpalH263Format;
typedef OpalMediaFormatStaticH323<OpalVideoFormat, H323H263plusCapability> OpalH263plusFormat;
#else
typedef OpalMediaFormatStatic<OpalVideoFormat> OpalH263Format;
typedef OpalMediaFormatStatic<OpalVideoFormat> OpalH263plusFormat;
#endif

const OpalVideoFormat & GetOpalH263()
{
  static OpalH263Format const format(new OpalH263FormatInternal(H263FormatName, H263EncodingName, RTP_DataFrame::H263));
  return format;
}


const OpalVideoFormat & GetOpalH263plus()
{
  static OpalH263plusFormat const format(new OpalH263FormatInternal(H263plusFormatName, H263plusEncodingName, RTP_DataFrame::DynamicBase));
  return format;
}


struct OpalKeyFrameDetectorH263 : OpalVideoFormat::FrameDetector
{
  virtual OpalVideoFormat::FrameType GetFrameType(const BYTE * rtp, PINDEX size)
  {
    if (size < 8)
      return OpalVideoFormat::e_UnknownFrameType;

    if ((rtp[4] & 0x1c) != 0x1c)
      return (rtp[4] & 2) != 0 ? OpalVideoFormat::e_InterFrame : OpalVideoFormat::e_IntraFrame;

    switch (((rtp[5] & 0x80) != 0 ? (rtp[7] >> 2) : (rtp[5] >> 5)) & 3) {
      case 0:
      case 4:
        return OpalVideoFormat::e_IntraFrame;
      case 1:
      case 5:
        return OpalVideoFormat::e_InterFrame;
    }
    return OpalVideoFormat::e_NonFrameBoundary;
  }
};


struct OpalKeyFrameDetectorRFC2190 : OpalKeyFrameDetectorH263
{
  virtual OpalVideoFormat::FrameType GetFrameType(const BYTE * rtp, PINDEX size)
  {
    // RFC 2190 header length
    static const PINDEX ModeLen[4] = { 4, 4, 8, 12 };
    PINDEX len = ModeLen[(rtp[0] & 0xC0) >> 6];
    if (size < len + 6)
      return OpalVideoFormat::e_UnknownFrameType;

    rtp += len;
    if (rtp[0] != 0 || rtp[1] != 0 || (rtp[2] & 0xfc) != 0x80)
      return OpalVideoFormat::e_NonFrameBoundary;

    return OpalKeyFrameDetectorH263::GetFrameType(rtp, size);
  }
};

PFACTORY_CREATE(OpalVideoFormat::FrameDetectFactory, OpalKeyFrameDetectorRFC2190, "H263");


struct OpalKeyFrameDetectorRFC4629 : OpalKeyFrameDetectorH263
{
  virtual OpalVideoFormat::FrameType GetFrameType(const BYTE * rtp, PINDEX size)
  {
    if (size < 6)
      return OpalVideoFormat::e_UnknownFrameType;

    if ((rtp[0] & 0xfd) != 4 || rtp[1] != 0 || (rtp[2] & 0xfc) != 0x80)
      return OpalVideoFormat::e_NonFrameBoundary;

    return OpalKeyFrameDetectorH263::GetFrameType(rtp, size);
  }
};

PFACTORY_CREATE(OpalVideoFormat::FrameDetectFactory, OpalKeyFrameDetectorRFC4629, "H263-1998");


#endif // OPAL_VIDEO

// End of File ///////////////////////////////////////////////////////////////
