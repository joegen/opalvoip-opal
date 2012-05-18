/*
 * g7221mf.cxx
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

#include <codec/opalplugin.h>
#include <codec/known.h>


///////////////////////////////////////////////////////////////////////////////

static const char G7221FormatName24K[] = OPAL_G7221_24K;
static const char G7221FormatName32K[] = OPAL_G7221_32K;
static const char G7221EncodingName[] = "G7221"; // MIME name rfc's 3047, 5577

#define G7221_24K_BIT_RATE         24000
#define G7221_32K_BIT_RATE         32000

#define G7221_SAMPLES_PER_FRAME    320   // 20 ms frame
#define G7221_SAMPLE_RATE          16000

static const char G7221BitRateOptionName[] = "Bit Rate";
static const char G7221BitRateFMTPName[]   = "bitrate";


// End of File ///////////////////////////////////////////////////////////////
