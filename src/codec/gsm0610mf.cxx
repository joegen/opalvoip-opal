/*
 * gsm0610mf.cxx
 *
 * GSM 06.10 Full Rate Media Format descriptions
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

const OpalAudioFormat & GetOpalGSM0610()
{
  static const OpalAudioFormat GSM0610(OPAL_GSM0610, RTP_DataFrame::GSM, "GSM",  33, 160, 7, 4, 7, 8000 );
  return GSM0610;
}

#endif


// End of File ///////////////////////////////////////////////////////////////
