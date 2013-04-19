/*
 * rfc2435.cxx
 *
 * RFC2435 JPEG transport for uncompressed video
 *
 * Open Phone Abstraction Library
 *
 * Copyright (C) 2010 Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rfc2435.h"
#endif

#include <opal_config.h>

#if OPAL_RFC2435

#include <ptclib/random.h>
#include <opal/mediafmt.h>
#include <codec/rfc2435.h>
#include <codec/opalplugin.h>


#define   FRAME_WIDTH   2040
#define   FRAME_HEIGHT  2040
#define   FRAME_RATE    60

#define   REASONABLE_UDP_PACKET_SIZE  800


#if defined(JPEGLIB_LIBRARY)
  #pragma comment(lib, JPEGLIB_LIBRARY)
#endif


class RFC2435VideoFormatInternal : public OpalVideoFormatInternal
{
  PCLASSINFO(RFC2435VideoFormatInternal, OpalVideoFormatInternal);
  public:
    RFC2435VideoFormatInternal(
      const char * fullName,    ///<  Full name of media format
      const char * samplingName,
      unsigned int bandwidth
    );

    virtual PObject * Clone() const
    {
      PWaitAndSignal m(media_format_mutex);
      return new RFC2435VideoFormatInternal(*this);
    }

    virtual bool ToNormalisedOptions()
    { 
      int v;
      if ((v = GetOptionInteger(OpalVideoFormat::FrameWidthOption(), -1)) > 0) {
        SetOptionInteger(OpalVideoFormat::MinRxFrameWidthOption(), v);
        SetOptionInteger(OpalVideoFormat::MaxRxFrameWidthOption(), v);
      }
      if ((v = GetOptionInteger(OpalVideoFormat::FrameHeightOption(), -1)) > 0) {
        SetOptionInteger(OpalVideoFormat::MinRxFrameHeightOption(), v);
        SetOptionInteger(OpalVideoFormat::MaxRxFrameHeightOption(), v);
      }
      return true;
    }
};

const OpalVideoFormat & GetOpalRFC2435_JPEG()
{
  static OpalVideoFormat RFC2435(new RFC2435VideoFormatInternal(OPAL_RFC2435_JPEG, "RFC2435_JPEG", (FRAME_WIDTH*FRAME_HEIGHT*3/2)*FRAME_RATE));
  return RFC2435;
}


/////////////////////////////////////////////////////////////////////////////

RFC2435VideoFormatInternal::RFC2435VideoFormatInternal(
      const char * fullName,    ///<  Full name of media format
      const char * samplingName,
      unsigned int bandwidth)
 : OpalVideoFormatInternal(fullName, 
                   RTP_DataFrame::DynamicBase,
                   "jpeg",
                   FRAME_WIDTH, FRAME_HEIGHT,
                   FRAME_RATE,
                   bandwidth, 0)
{
  OpalMediaOption * option;

#if OPAL_SIP
  // add mandatory fields
  option = FindOption(OpalVideoFormat::ClockRateOption());
  if (option != NULL)
    option->SetFMTPName("rate");

  option = FindOption(OpalVideoFormat::FrameWidthOption());
  if (option != NULL)
    option->SetFMTPName("width");

  option = FindOption(OpalVideoFormat::FrameHeightOption());
  if (option != NULL)
    option->SetFMTPName("height");
#endif // OPAL_SIP
}

/////////////////////////////////////////////////////////////////////////////

OpalRFC2435Encoder::OpalRFC2435Encoder()
  : OpalVideoTranscoder(OpalYUV420P, OpalRFC2435_JPEG)
{
}

bool OpalRFC2435Encoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & outputFrames)
{
  outputFrames.RemoveAll();
  return false;
}


/////////////////////////////////////////////////////////////////////////////

OpalRFC2435Decoder::OpalRFC2435Decoder()
  : OpalVideoTranscoder(OpalRFC2435_JPEG, OpalYUV420P)
{
}


bool OpalRFC2435Decoder::ConvertFrames(const RTP_DataFrame & input, RTP_DataFrameList & output)
{
  output.RemoveAll();
  return false;
}

#endif // OPAL_RFC2435


