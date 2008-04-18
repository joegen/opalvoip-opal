/*
 * g726mf.cxx
 *
 * G.726 Media Format descriptions
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

const OpalAudioFormat & GetOpalG726_40K()
{
  static const OpalAudioFormat G726_40K(OPAL_G726_40K, RTP_DataFrame::DynamicBase,  "G726-40", 5, 8, 240, 30, 256, 8000);
  return G726_40K;
}

const OpalAudioFormat & GetOpalG726_32K()
{
  static const OpalAudioFormat G726_32K(OPAL_G726_32K, RTP_DataFrame::DynamicBase,  "G726-32", 4, 8, 240, 30, 256, 8000);
  return G726_32K;
}

const OpalAudioFormat & GetOpalG726_24K()
{
  static const OpalAudioFormat G726_24K(OPAL_G726_24K, RTP_DataFrame::DynamicBase,  "G726-24", 3, 8, 240, 30, 256, 8000);
  return G726_24K;
}

const OpalAudioFormat & GetOpalG726_16K()
{
  static const OpalAudioFormat G726_16K(OPAL_G726_16K, RTP_DataFrame::DynamicBase,  "G726-16", 2, 8, 240, 30, 256, 8000);
  return G726_16K;
}

#endif


// End of File ///////////////////////////////////////////////////////////////
