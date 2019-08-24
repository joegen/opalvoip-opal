/*
 * iLBCmf.cxx
 *
 * iLBC Media Format descriptions
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

#include <opal/mediafmt.h>
#include <codec/opalplugin.h>
#include <h323/h323caps.h>
#include <asn/h245.h>


#define FRAME_SIZE_NB  160
#define SAMPLE_RATE_NB 8000

#define FRAME_SIZE_WB  320
#define SAMPLE_RATE_WB 16000


/////////////////////////////////////////////////////////////////////////////

class OpalSpeexFormat : public OpalAudioFormatInternal
{
    PCLASSINFO_WITH_CLONE(OpalSpeexFormat, OpalAudioFormatInternal);
  public:
    OpalSpeexFormat(const char * name, unsigned frameSize, unsigned sampleRate)
      : OpalAudioFormatInternal(name, RTP_DataFrame::DynamicBase, "speex",  50, frameSize, 1, 1, 1, sampleRate, 0)
    {
#if OPAL_SDP
      OpalMediaOption * option = new OpalMediaOptionString("mode", false);
      option->SetFMTP("mode", "3,any");
      AddOption(option);

      const char * const VBR[] = { "off", "on", "vad" };
      option = new OpalMediaOptionEnum("Variable Bitrate", false, VBR, PARRAYSIZE(VBR), OpalMediaOption::MinMerge);
      option->SetFMTP("vbr", VBR[0]);
      AddOption(option);

      const char * const CNG[] = { "off", "on" };
      option = new OpalMediaOptionEnum("Comfort Noise", false, CNG, PARRAYSIZE(CNG), OpalMediaOption::MinMerge);
      option->SetFMTP("cng", CNG[0]);
      AddOption(option);
#endif
    }
};


#define FORMAT(type) \
  const OpalAudioFormat & GetOpalSpeex##type() \
  { \
    static OpalMediaFormatStatic<OpalAudioFormat> const format(PNEW OpalSpeexFormat(OPAL_SPEEX_##type, FRAME_SIZE_##type, SAMPLE_RATE_##type)); \
    return format; \
  }

FORMAT(NB);
FORMAT(WB);


// End of File ///////////////////////////////////////////////////////////////
