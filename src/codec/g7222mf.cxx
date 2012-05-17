/*
 * g7222mf.cxx
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

// This is to get around the DevStudio precompiled headers rqeuirements.
#ifdef OPAL_PLUGIN_COMPILE
#define PTLIB_PTLIB_H
#endif

#include <ptlib.h>

#include <codec/opalplugin.h>
#include <codec/known.h>


///////////////////////////////////////////////////////////////////////////////

static const char G7222FormatName[]   = OPAL_G7222;
static const char G7222EncodingName[] = "AMR-WB";

#define G7222_SAMPLES_PER_FRAME    320   // 20 ms frame
#define G7222_SAMPLE_RATE          16000
#define G7222_MAX_BYTES_PER_FRAME  62
#define G7222_MAX_BIT_RATE         (G7222_MAX_BYTES_PER_FRAME*400)

static const char G7222InitialModeName[]               = "Initial Mode";
static const char G7222AlignmentOptionName[]           = "Octet Aligned";
static const char G7222ModeSetOptionName[]             = "Mode Set";
static const char G7222ModeChangePeriodOptionName[]    = "Mode Change Period";
static const char G7222ModeChangeNeighbourOptionName[] = "Mode Change Neighbour";
static const char G7222CRCOptionName[]                 = "CRC";
static const char G7222RobustSortingOptionName[]       = "Robust Sorting";
static const char G7222InterleavingOptionName[]        = "Interleaving";

// H.245 generic parameters; see G.722.2
enum
{
    G7222_H245_MAXAL_SDUFRAMES_RX    = 0 | PluginCodec_H245_Collapsing   | PluginCodec_H245_TCS,
    G7222_H245_MAXAL_SDUFRAMES_TX    = 0 | PluginCodec_H245_Collapsing   | PluginCodec_H245_OLC,
    G7222_H245_REQUEST_MODE          = 1 | PluginCodec_H245_NonCollapsing| PluginCodec_H245_ReqMode,
    G7222_H245_OCTET_ALIGNED         = 2 | PluginCodec_H245_Collapsing   | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    G7222_H245_MODE_SET              = 3 | PluginCodec_H245_Collapsing   | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    G7222_H245_MODE_CHANGE_PERIOD    = 4 | PluginCodec_H245_Collapsing   | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    G7222_H245_MODE_CHANGE_NEIGHBOUR = 5 | PluginCodec_H245_Collapsing   | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    G7222_H245_CRC                   = 6 | PluginCodec_H245_Collapsing   | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    G7222_H245_ROBUST_SORTING        = 7 | PluginCodec_H245_Collapsing   | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
    G7222_H245_INTERLEAVING          = 8 | PluginCodec_H245_Collapsing   | PluginCodec_H245_TCS | PluginCodec_H245_OLC | PluginCodec_H245_ReqMode,
};


static const char G7222InitialModeFMTPName[] = "mode";
static const char G7222AlignmentFMTPName[]   = "octet-align";

#define G7222_MAX_MODES              8
#define G7222_MODE_INITIAL_VALUE     7
#define G7222_MODE_SET_INITIAL_VALUE 0x1ff


/////////////////////////////////////////////////////////////////////////////

#ifndef OPAL_PLUGIN_COMPILE

#include <opal/mediafmt.h>
#include <h323/h323caps.h>


class OpalG7222Format : public OpalAudioFormatInternal
{
  public:
    OpalG7222Format()
      : OpalAudioFormatInternal(G7222FormatName,
                                RTP_DataFrame::DynamicBase,
                                G7222EncodingName,
                                G7222_MAX_BYTES_PER_FRAME,
                                G7222_SAMPLES_PER_FRAME,
                                1, 1, 1,
                                G7222_SAMPLE_RATE)
    {
      OpalMediaOption * option = new OpalMediaOptionInteger(G7222InitialModeName,
                                                            false,
                                                            OpalMediaOption::MinMerge,
                                                            G7222_MODE_INITIAL_VALUE,
                                                            0, G7222_MAX_MODES);
      OPAL_SET_MEDIA_OPTION_FMTP(option, G7222InitialModeFMTPName, "0");
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_REQUEST_MODE);
      AddOption(option);

      option = new OpalMediaOptionBoolean(G7222AlignmentOptionName, false, OpalMediaOption::AndMerge, false);
      OPAL_SET_MEDIA_OPTION_FMTP(option, G7222AlignmentFMTPName, "0");
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_OCTET_ALIGNED);
      AddOption(option);

#if OPAL_H323
      option = FindOption(OpalAudioFormat::RxFramesPerPacketOption());
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_MAXAL_SDUFRAMES_RX);

      option = FindOption(OpalAudioFormat::TxFramesPerPacketOption());
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_MAXAL_SDUFRAMES_TX);

      option = new OpalMediaOptionInteger(G7222ModeSetOptionName, true, OpalMediaOption::EqualMerge, G7222_MODE_SET_INITIAL_VALUE);
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_MODE_SET);
      AddOption(option);

      option = new OpalMediaOptionInteger(G7222ModeChangePeriodOptionName, false, OpalMediaOption::MinMerge, 0, 1000);
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_MODE_CHANGE_PERIOD);
      AddOption(option);

      option = new OpalMediaOptionBoolean(G7222ModeChangeNeighbourOptionName, false, OpalMediaOption::AndMerge);
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_MODE_CHANGE_NEIGHBOUR);
      AddOption(option);

      option = new OpalMediaOptionBoolean(G7222CRCOptionName, true, OpalMediaOption::AndMerge);
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_CRC);
      AddOption(option);

      option = new OpalMediaOptionBoolean(G7222RobustSortingOptionName, true, OpalMediaOption::AndMerge);
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_ROBUST_SORTING);
      AddOption(option);

      option = new OpalMediaOptionBoolean(G7222InterleavingOptionName, true, OpalMediaOption::AndMerge);
      OPAL_SET_MEDIA_OPTION_H245(option, G7222_H245_INTERLEAVING);
      AddOption(option);
#endif // OPAL_H323

      AddOption(new OpalMediaOptionString(PLUGINCODEC_MEDIA_PACKETIZATIONS, true, "RFC3267,RFC4867"));
    }
};


#if OPAL_H323
extern const char G7222_Identifier[] = OpalPluginCodec_Identifer_G7222;
#endif


const OpalAudioFormat & GetOpalG7222()
{
  static OpalAudioFormat const format(new OpalG7222Format);

#if OPAL_H323
  static H323CapabilityFactory::Worker<
    H323GenericAudioCapabilityTemplate<G7222_Identifier, GetOpalG7222>
  > capability(G7222FormatName, true);
#endif

  return format;
}


#endif // OPAL_PLUGIN_COMPILE


// End of File ///////////////////////////////////////////////////////////////
