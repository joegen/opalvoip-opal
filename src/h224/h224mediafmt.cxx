/*
 * h224mediafmt.cxx
 *
 * H.224 Media Format implementation for the OpenH323 Project.
 *
 * Copyright (c) 2007 Network for Educational Technology, ETH Zurich.
 * Written by Hannes Friederich.
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
 * Contributor(s): ______________________________________.
 *
 * $Log: h224mediafmt.cxx,v $
 * Revision 1.1.2.2  2007/03/20 00:02:13  hfriederich
 * (Backport from HEAD)
 * Add ability to remove H.224
 *
 * Revision 1.1.2.1  2007/02/15 15:25:36  hfriederich
 * Add media format and media type for H.224
 *
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h224mediafmt.h"
#endif

#include <opal/buildopts.h>

#if OPAL_H224

#include <h224/h224mediafmt.h>

const OpalMediaFormat & GetOpalH224()
{
  static const OpalMediaType H224MediaType(
    "H224Media",
    OpalMediaType::Application
  );
  static const OpalMediaFormat H224(
    OPAL_H224_NAME,
    H224MediaType,
    (RTP_DataFrame::PayloadTypes)100, // Most other implementations seem to use this payload code
    NULL, //"H224", // As defined in RFC 4573 (Currently disabled for SIP as long there is no standardized signaling)
    FALSE,
    6400, // 6.4kbit/s as defined in RFC 4573
    0,
    0,
    4800  // As defined in RFC 4573
  );
  return H224;
}

#endif // OPAL_H224
