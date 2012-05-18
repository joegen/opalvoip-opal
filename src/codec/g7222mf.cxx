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

#include <ptlib.h>

#include "g7222mf_inc.cxx"

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


// End of File ///////////////////////////////////////////////////////////////
