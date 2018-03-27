/*
 * t38mf.cxx
 *
 * T.38 Media Format descriptions
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

#include <t38/t38proto.h>
#include <opal/mediafmt.h>
#include <opal/mediasession.h>


#define new PNEW


#if OPAL_T38_CAPABILITY

#include <rtp/rtp.h>

OPAL_MEDIATYPE(OpalFaxMedia);

const PCaselessString & OpalFaxMediaDefinition::UDPTL() { static const PConstCaselessString s("udptl"); return s; }


/////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalT38()
{
  class OpalT38MediaFormatInternal : public OpalMediaFormatInternal {
    public:
      OpalT38MediaFormatInternal()
        : OpalMediaFormatInternal(OPAL_T38,
                          OpalFaxMediaType(),
                          RTP_DataFrame::T38,
                          "t38",
                          false, // No jitter for data
                          1440, // 100's bits/sec
                          528,
                          0, 0, 0)
      {
        SetOptionString(OpalMediaFormat::DescriptionOption(), "ITU-T T.38 Group 3 facsimile");

        static const char * const RateMan[] = { OPAL_T38localTCF, OPAL_T38transferredTCF };
        AddOption(new OpalMediaOptionEnum(OPAL_T38FaxRateManagement, false, RateMan, PARRAYSIZE(RateMan), OpalMediaOption::EqualMerge, 1));
        AddOption(new OpalMediaOptionInteger(OPAL_T38FaxVersion, false, OpalMediaOption::MinMerge, 0, 0, 1));
        AddOption(new OpalMediaOptionInteger(OPAL_T38MaxBitRate, false, OpalMediaOption::NoMerge, 14400, 1200, 14400));
        AddOption(new OpalMediaOptionInteger(OPAL_T38FaxMaxBuffer, false, OpalMediaOption::NoMerge, 2000, 10, 65535));
        AddOption(new OpalMediaOptionInteger(OPAL_T38FaxMaxDatagram, false, OpalMediaOption::NoMerge, 528, 10, 65535));
        static const char * const UdpEC[] = { OPAL_T38UDPFEC, OPAL_T38UDPRedundancy };
        AddOption(new OpalMediaOptionEnum(OPAL_T38FaxUdpEC, false, UdpEC, PARRAYSIZE(UdpEC), OpalMediaOption::AlwaysMerge, 1));
        AddOption(new OpalMediaOptionBoolean(OPAL_T38FaxFillBitRemoval, false, OpalMediaOption::NoMerge, false));
        AddOption(new OpalMediaOptionBoolean(OPAL_T38FaxTranscodingMMR, false, OpalMediaOption::NoMerge, false));
        AddOption(new OpalMediaOptionBoolean(OPAL_T38FaxTranscodingJBIG, false, OpalMediaOption::NoMerge, false));
        AddOption(new OpalMediaOptionBoolean(OPAL_T38UseECM, false, OpalMediaOption::NoMerge, true));
        AddOption(new OpalMediaOptionString(OPAL_FaxStationIdentifier, false, "-"));
        AddOption(new OpalMediaOptionString(OPAL_FaxHeaderInfo, false));
        AddOption(new OpalMediaOptionBoolean(OPAL_UDPTLRawMode, false, OpalMediaOption::NoMerge, false));
        AddOption(new OpalMediaOptionString(OPAL_UDPTLRedundancy, false));
        AddOption(new OpalMediaOptionInteger(OPAL_UDPTLRedundancyInterval, false, OpalMediaOption::NoMerge, 0, 0, 86400));
        AddOption(new OpalMediaOptionBoolean(OPAL_UDPTLOptimiseRetransmit, false, OpalMediaOption::NoMerge, false));
        AddOption(new OpalMediaOptionInteger(OPAL_UDPTLKeepAliveInterval, false, OpalMediaOption::NoMerge, 0, 0, 86400));
      }
  };
  static OpalMediaFormatStatic<OpalMediaFormat> T38(new OpalT38MediaFormatInternal);
  return T38;
}


/////////////////////////////////////////////////////////////////////////////

const char * OpalFaxMediaDefinition::Name() { return "fax"; }

OpalFaxMediaDefinition::OpalFaxMediaDefinition()
  : OpalMediaTypeDefinition(Name(), UDPTL(), 3) // Must be 3 for H.323 operation
{
}


#endif // OPAL_T38_CAPABILITY


// End of File ///////////////////////////////////////////////////////////////
