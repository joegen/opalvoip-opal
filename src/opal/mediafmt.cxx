/*
 * mediafmt.cxx
 *
 * Media Format descriptions
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: mediafmt.cxx,v $
 * Revision 1.2001  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.3  2001/05/11 04:43:43  robertj
 * Added variable names for standard PCM-16 media format name.
 *
 * Revision 1.2  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.1  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediafmt.h"
#endif

#include <opal/mediafmt.h>


#define new PNEW


///////////////////////////////////////////////////////////////////////////////

OpalMediaFormat const OpalPCM16(
  OPAL_PCM16,
  RTP_Session::DefaultAudioSessionID,
  RTP_DataFrame::L16_Mono,
  TRUE,   // Needs jitter
  128000, // bits/sec
  16, // bytes/frame
  8, // 1 millisecond
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG711uLaw(
  OPAL_G711_ULAW_64K,
  RTP_Session::DefaultAudioSessionID,
  RTP_DataFrame::PCMU,
  TRUE,   // Needs jitter
  64000, // bits/sec
  8, // bytes/frame
  8, // 1 millisecond/frame
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG711ALaw(
  OPAL_G711_ALAW_64K,
  RTP_Session::DefaultAudioSessionID,
  RTP_DataFrame::PCMA,
  TRUE,   // Needs jitter
  64000, // bits/sec
  8, // bytes/frame
  8, // 1 millisecond/frame
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG728(  
  OPAL_G728,
  RTP_Session::DefaultAudioSessionID,
  RTP_DataFrame::G728,
  TRUE, // Needs jitter
  16000,// bits/sec
  5,    // bytes
  20,   // 2.5 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG729A( 
  OPAL_G729A,
  RTP_Session::DefaultAudioSessionID,
  RTP_DataFrame::G729,
  TRUE, // Needs jitter
  8000, // bits/sec
  10,   // bytes
  80,   // 10 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG729AB(
  OPAL_G729AB,
  RTP_Session::DefaultAudioSessionID,
  RTP_DataFrame::G729,
  TRUE, // Needs jitter
  8000, // bits/sec
  10,   // bytes
  80,   // 10 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG7231(
  OPAL_G7231,
  RTP_Session::DefaultAudioSessionID,
  RTP_DataFrame::G7231,
  TRUE, // Needs jitter
  6400, // bits/sec
  24,   // bytes
  240,  // 30 milliseconds
  OpalMediaFormat::AudioTimeUnits
);


/////////////////////////////////////////////////////////////////////////////

OpalMediaFormat::OpalMediaFormat()
{
  rtpPayloadType = RTP_DataFrame::MaxPayloadType;

  needsJitter = FALSE;
  bandwidth = 0;
  frameSize = 0;
  frameTime = 0;
  timeUnits = 0;
}


OpalMediaFormat::OpalMediaFormat(const char * search, BOOL exact)
  : PCaselessString(search)
{
  rtpPayloadType = RTP_DataFrame::MaxPayloadType;

  needsJitter = FALSE;
  bandwidth = 0;
  frameSize = 0;
  frameTime = 0;
  timeUnits = 0;

  const List & registeredFormats = GetRegisteredMediaFormats();
  for (PINDEX i = 0; i < registeredFormats.GetSize(); i++) {
    if (exact ? (registeredFormats[i] == search)
              : (registeredFormats[i].Find(search) != P_MAX_INDEX)) {
      *this = registeredFormats[i];
      return;
    }
  }
}


OpalMediaFormat::OpalMediaFormat(const char * fullName,
                                 unsigned id,
                                 RTP_DataFrame::PayloadTypes pt,
                                 BOOL     nj,
                                 unsigned bw,
                                 PINDEX   fs,
                                 unsigned ft,
                                 unsigned tu)
  : PCaselessString(fullName)
{
  rtpPayloadType = pt;
  defaultSessionID = id;
  needsJitter = nj;
  bandwidth = bw;
  frameSize = fs;
  frameTime = ft;
  timeUnits = tu;

  PINDEX i;
  List & registeredFormats = GetMediaFormatsList();

  if ((i = registeredFormats.GetValuesIndex(*this)) != P_MAX_INDEX) {
    *this = registeredFormats[i]; // Already registered, use previous values
    return;
  }

  if (rtpPayloadType == RTP_DataFrame::DynamicBase) {
    // Are dynamic RTP payload, find unique value in registered list.
    do {
      for (i = 0; i < registeredFormats.GetSize(); i++) {
        if (registeredFormats[i].GetPayloadType() == rtpPayloadType) {
          rtpPayloadType = (RTP_DataFrame::PayloadTypes)(rtpPayloadType+1);
          break;
        }
      }
    } while (i < registeredFormats.GetSize());
  }

  registeredFormats.Append(this);
}


OpalMediaFormat & OpalMediaFormat::operator=(const char * search)
{
  rtpPayloadType = RTP_DataFrame::MaxPayloadType;

  needsJitter = FALSE;
  bandwidth = 0;
  frameSize = 0;
  frameTime = 0;
  timeUnits = 0;

  const List & registeredFormats = GetRegisteredMediaFormats();
  for (PINDEX i = 0; i < registeredFormats.GetSize(); i++) {
    if (registeredFormats[i] == search) {
      *this = registeredFormats[i];
      break;
    }
  }

  return *this;
}


OpalMediaFormat::List & OpalMediaFormat::GetMediaFormatsList()
{
  static List registeredFormats(TRUE);
  return registeredFormats;
}


OpalMediaFormat::List::List(BOOL disallow)
{
  if (disallow)
    DisallowDeleteObjects();
}


OpalMediaFormat::List & OpalMediaFormat::List::operator+=(const OpalMediaFormat & format)
{
  if (format.IsValid()) {
    PINDEX idx = GetValuesIndex(format);
    if (idx == P_MAX_INDEX)
      Append(new OpalMediaFormat(format));
  }

  return *this;
}


OpalMediaFormat::List & OpalMediaFormat::List::operator-=(const OpalMediaFormat & format)
{
  PINDEX idx = GetValuesIndex(format);
  if (idx != P_MAX_INDEX)
    RemoveAt(idx);

  return *this;
}


// End of File ///////////////////////////////////////////////////////////////
