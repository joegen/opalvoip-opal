/*
 * gsmamrmf.cxx
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

#include <opal/mediafmt.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

const OpalAudioFormat & GetOpalGSMAMR()
{
  static class OpalGSMAMRFormat : public OpalAudioFormat
  {
    public:
      OpalGSMAMRFormat()
        : OpalAudioFormat(OPAL_GSMAMR, RTP_DataFrame::DynamicBase, "AMR",  33, 160, 1, 1, 1, 8000)
      {
        AddOption(new OpalMediaOptionInteger("Initial Mode", false, OpalMediaOption::MinMerge, 7));
        AddOption(new OpalMediaOptionBoolean("VAD", false, OpalMediaOption::AndMerge, true));
        AddOption(new OpalMediaOptionString("Media Packetization", true, "RFC3267"));
      }
  } const GSMAMR;
  return GSMAMR;
}

#endif


// End of File ///////////////////////////////////////////////////////////////
