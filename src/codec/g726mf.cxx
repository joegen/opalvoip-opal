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
#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include <h323/h323caps.h>
#include <asn/h245.h>
#include <codec/opalplugin.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

#if OPAL_H323
#define IDENTIFIER(rate) extern const char G726_##rate##K_Identifier[] = "0.0.7.726.1.0." #rate;

#define CAPABILITY(rate) \
  static H323CapabilityFactory::Worker< \
  H323GenericAudioCapabilityTemplate<G726_##rate##K_Identifier, GetOpalG726_##rate##K, rate##000> \
  > capability(G726_##rate##K_FormatName, true)

#else
#define IDENTIFIER(rate)
#define CAPABILITY(rate)
#endif // OPAL_H323


/////////////////////////////////////////////////////////////////////////////

#define FORMAT(rate, bits) \
  IDENTIFIER(rate) \
  static const char G726_##rate##K_FormatName[] = OPAL_G726_##rate##K; \
  static const char G726_##rate##K_EncodingName[] = "G726-" #rate; \
  const OpalAudioFormat & GetOpalG726_##rate##K() \
  { \
    static const OpalAudioFormat format(G726_##rate##K_FormatName, RTP_DataFrame::DynamicBase, \
                                        G726_##rate##K_EncodingName, bits, 8, 240, 30, 256, 8000); \
    CAPABILITY(rate); \
    return format; \
  }

FORMAT(40, 5);
FORMAT(32, 4);
FORMAT(24, 3);
FORMAT(16, 2);


// End of File ///////////////////////////////////////////////////////////////
