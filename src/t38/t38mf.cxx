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


#if OPAL_SDP
  #define ADD_OPTION(cls, name, ...) { \
    cls * p = new cls(name, __VA_ARGS__); \
    p->SetFMTP(name, "false"); \
    mf.AddOption(p); \
  }
#else
  #define ADD_OPTION(cls, name, ...) mf.AddOption(new cls(name, __VA_ARGS__))
#endif

static void AddOptions(OpalMediaFormatInternal & mf)
{
    mf.SetOptionString(OpalMediaFormat::DescriptionOption(), "ITU-T T.38 Group 3 facsimile");

    static const char * const RateMan[] = { OPAL_T38localTCF, OPAL_T38transferredTCF };
    ADD_OPTION(OpalMediaOptionEnum,    OPAL_T38FaxRateManagement, false, RateMan, PARRAYSIZE(RateMan), OpalMediaOption::EqualMerge, 1);
    ADD_OPTION(OpalMediaOptionInteger, OPAL_T38FaxVersion, false, OpalMediaOption::MinMerge, 0, 0, 1);
    ADD_OPTION(OpalMediaOptionInteger, OPAL_T38MaxBitRate, false, OpalMediaOption::NoMerge, 14400, 1200, 14400);
    ADD_OPTION(OpalMediaOptionInteger, OPAL_T38FaxMaxBuffer, false, OpalMediaOption::NoMerge, 2000, 10, 65535);
    ADD_OPTION(OpalMediaOptionInteger, OPAL_T38FaxMaxDatagram, false, OpalMediaOption::NoMerge, 528, 10, 65535);
    ADD_OPTION(OpalMediaOptionBoolean, OPAL_T38FaxFillBitRemoval, false, OpalMediaOption::NoMerge, false);
    ADD_OPTION(OpalMediaOptionBoolean, OPAL_T38FaxTranscodingMMR, false, OpalMediaOption::NoMerge, false);
    ADD_OPTION(OpalMediaOptionBoolean, OPAL_T38FaxTranscodingJBIG, false, OpalMediaOption::NoMerge, false);
    ADD_OPTION(OpalMediaOptionBoolean, OPAL_T38UseECM, false, OpalMediaOption::NoMerge, true);
    ADD_OPTION(OpalMediaOptionString,  OPAL_FaxStationIdentifier, false, "-");
    ADD_OPTION(OpalMediaOptionString,  OPAL_FaxHeaderInfo, false);
}

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
        AddOptions(*this);

        static const char * const UdpEC[] = { OPAL_T38UDPFEC, OPAL_T38UDPRedundancy };
        AddOption(new OpalMediaOptionEnum(OPAL_T38FaxUdpEC, false, UdpEC, PARRAYSIZE(UdpEC), OpalMediaOption::AlwaysMerge, 1));
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


const OpalMediaFormat & GetOpalT38_RTP()
{
  class OpalT38RTPMediaFormatInternal : public OpalMediaFormatInternal {
    public:
      OpalT38RTPMediaFormatInternal()
        : OpalMediaFormatInternal(OPAL_T38_RTP,
                                  OpalMediaType::Audio(),
                                  RTP_DataFrame::DynamicBase,
                                  "t38",
                                  false, // No jitter for data
                                  1440, // 100's bits/sec
                                  528,
                                  0, 0, 0)
      {
        AddOptions(*this);
      }
  };
  static OpalMediaFormatStatic<OpalMediaFormat> T38_RTP(new OpalT38RTPMediaFormatInternal);
  return T38_RTP;
}


#endif // OPAL_T38_CAPABILITY


// End of File ///////////////////////////////////////////////////////////////
