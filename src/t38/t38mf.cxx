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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#include <t38/t38proto.h>
#include <opal/mediafmt.h>
#include <opal/mediasession.h>


#define new PNEW


#if OPAL_T38_CAPABILITY

#include <rtp/rtp.h>

OPAL_INSTANTIATE_MEDIATYPE(OpalFaxMediaType);

const PCaselessString & OpalFaxMediaType::UDPTL() { static const PConstCaselessString s("udptl"); return s; }


/////////////////////////////////////////////////////////////////////////////

const OpalMediaFormat & GetOpalT38()
{
  static class T38MediaFormat : public OpalMediaFormat {
    public:
      T38MediaFormat()
        : OpalMediaFormat(OPAL_T38,
                          OpalFaxMediaType::Name(),
                          RTP_DataFrame::T38,
                          "t38",
                          false, // No jitter for data
                          1440, // 100's bits/sec
                          528,
                          0,
                          0)
      {
        static const char * const RateMan[] = { "localTCF", "transferredTCF" };
        AddOption(new OpalMediaOptionEnum("T38FaxRateManagement", false, RateMan, PARRAYSIZE(RateMan), OpalMediaOption::EqualMerge, 1));
        AddOption(new OpalMediaOptionInteger("T38FaxVersion", false, OpalMediaOption::MinMerge, 0, 0, 1));
        AddOption(new OpalMediaOptionInteger("T38MaxBitRate", false, OpalMediaOption::NoMerge, 14400, 1200, 14400));
        AddOption(new OpalMediaOptionInteger("T38FaxMaxBuffer", false, OpalMediaOption::NoMerge, 2000, 10, 65535));
        AddOption(new OpalMediaOptionInteger("T38FaxMaxDatagram", false, OpalMediaOption::NoMerge, 528, 10, 65535));
        static const char * const UdpEC[] = { "t38UDPFEC", "t38UDPRedundancy" };
        AddOption(new OpalMediaOptionEnum("T38FaxUdpEC", false, UdpEC, PARRAYSIZE(UdpEC), OpalMediaOption::AlwaysMerge, 1));
        AddOption(new OpalMediaOptionBoolean("T38FaxFillBitRemoval", false, OpalMediaOption::NoMerge, false));
        AddOption(new OpalMediaOptionBoolean("T38FaxTranscodingMMR", false, OpalMediaOption::NoMerge, false));
        AddOption(new OpalMediaOptionBoolean("T38FaxTranscodingJBIG", false, OpalMediaOption::NoMerge, false));
        AddOption(new OpalMediaOptionBoolean("Use-ECM", false, OpalMediaOption::NoMerge, true));
        AddOption(new OpalMediaOptionString("Station-Identifier", false, "-"));
        AddOption(new OpalMediaOptionString("Header-Info", false));
      }
  } const T38;
  return T38;
}


/////////////////////////////////////////////////////////////////////////////

const char * OpalFaxMediaType::Name() { return "fax"; }

OpalFaxMediaType::OpalFaxMediaType()
  : OpalMediaTypeDefinition(Name(), UDPTL(), 3) // Must be 3 for H.323 operation
{
}


#endif // OPAL_T38_CAPABILITY


// End of File ///////////////////////////////////////////////////////////////
