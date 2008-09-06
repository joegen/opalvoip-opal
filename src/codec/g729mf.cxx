/*
 * g729mf.cxx
 *
 * G.729 Media Format descriptions
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

#include <opal/mediafmt.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

class OpalG729Format : public OpalAudioFormat
{
  public:
    OpalG729Format(const char * variant)
      : OpalAudioFormat(variant, RTP_DataFrame::G729, "G729", 10, 80, 24, 5, 256, 8000)
    {
#ifdef OPAL_SIP
      // As per RFC3555
      bool isAnnexB = variant[strlen(variant)-1] == 'B';
      static const char * const yesno[] = { "no", "yes" };
      OpalMediaOption * option = new OpalMediaOptionEnum("VAD", true, yesno, 2, OpalMediaOption::AndMerge, isAnnexB);
      option->SetFMTPName("annexb");
      option->SetFMTPDefault("yes");
      AddOption(option);
#endif
    }
};


#define AUDIO_FORMAT(name) \
  const OpalAudioFormat & GetOpal##name() \
  { \
    static const OpalG729Format name(OPAL_##name); \
    return name; \
  }

AUDIO_FORMAT(G729);
AUDIO_FORMAT(G729A);
AUDIO_FORMAT(G729B);
AUDIO_FORMAT(G729AB);


// End of File ///////////////////////////////////////////////////////////////
