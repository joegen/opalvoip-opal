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
 * Revision 1.2005  2001/08/23 05:51:17  robertj
 * Completed implementation of codec reordering.
 *
 * Revision 2.3  2001/08/22 03:51:44  robertj
 * Added functions to look up media format by payload type.
 *
 * Revision 2.2  2001/08/01 06:22:07  robertj
 * Fixed GNU warning.
 *
 * Revision 2.1  2001/08/01 05:45:34  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
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
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::L16_Mono,
  TRUE,   // Needs jitter
  128000, // bits/sec
  16, // bytes/frame
  8, // 1 millisecond
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG711uLaw(
  OPAL_G711_ULAW_64K,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::PCMU,
  TRUE,   // Needs jitter
  64000, // bits/sec
  8, // bytes/frame
  8, // 1 millisecond/frame
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG711ALaw(
  OPAL_G711_ALAW_64K,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::PCMA,
  TRUE,   // Needs jitter
  64000, // bits/sec
  8, // bytes/frame
  8, // 1 millisecond/frame
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG728(  
  OPAL_G728,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::G728,
  TRUE, // Needs jitter
  16000,// bits/sec
  5,    // bytes
  20,   // 2.5 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG729( 
  OPAL_G729,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::G729,
  TRUE, // Needs jitter
  8000, // bits/sec
  10,   // bytes
  80,   // 10 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG729A( 
  OPAL_G729A,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::G729,
  TRUE, // Needs jitter
  8000, // bits/sec
  10,   // bytes
  80,   // 10 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG729B(
  OPAL_G729B,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::G729,
  TRUE, // Needs jitter
  8000, // bits/sec
  10,   // bytes
  80,   // 10 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG729AB(
  OPAL_G729AB,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::G729,
  TRUE, // Needs jitter
  8000, // bits/sec
  10,   // bytes
  80,   // 10 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalG7231(
  OPAL_G7231,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::G7231,
  TRUE, // Needs jitter
  6400, // bits/sec
  24,   // bytes
  240,  // 30 milliseconds
  OpalMediaFormat::AudioTimeUnits
);

OpalMediaFormat const OpalGSM0610(
  OPAL_GSM0610,
  OpalMediaFormat::DefaultAudioSessionID,
  RTP_DataFrame::GSM,
  TRUE,  // Needs jitter
  13200, // bits/sec
  33,    // bytes
  160,   // 20 milliseconds
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


OpalMediaFormat::OpalMediaFormat(RTP_DataFrame::PayloadTypes pt)
{
  operator=(pt);
}


OpalMediaFormat::OpalMediaFormat(const char * wildcard)
{
  operator=(PString(wildcard));
}


OpalMediaFormat::OpalMediaFormat(const PString & wildcard)
{
  operator=(wildcard);
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
  OpalMediaFormatList & registeredFormats = GetMediaFormatsList();

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

  registeredFormats.OpalMediaFormatBaseList::Append(this);
}


OpalMediaFormat & OpalMediaFormat::operator=(RTP_DataFrame::PayloadTypes pt)
{
  const OpalMediaFormatList & registeredFormats = GetRegisteredMediaFormats();
  PINDEX idx = registeredFormats.FindFormat(pt);
  if (idx != P_MAX_INDEX)
    *this = registeredFormats[idx];
  else
    *this = OpalMediaFormat();

  return *this;
}


OpalMediaFormat & OpalMediaFormat::operator=(const char * wildcard)
{
  return operator=(PString(wildcard));
}


OpalMediaFormat & OpalMediaFormat::operator=(const PString & wildcard)
{
  const OpalMediaFormatList & registeredFormats = GetRegisteredMediaFormats();
  PINDEX idx = registeredFormats.FindFormat(wildcard);
  if (idx != P_MAX_INDEX)
    *this = registeredFormats[idx];
  else
    *this = OpalMediaFormat();

  return *this;
}


OpalMediaFormatList & OpalMediaFormat::GetMediaFormatsList()
{
  static OpalMediaFormatList registeredFormats;
  return registeredFormats;
}


OpalMediaFormatList::OpalMediaFormatList()
{
  DisallowDeleteObjects();
}


OpalMediaFormatList & OpalMediaFormatList::operator+=(const OpalMediaFormat & format)
{
  if (!format) {
    if (!HasFormat(format)) {
      const OpalMediaFormatList & registeredFormats = OpalMediaFormat::GetRegisteredMediaFormats();
      PINDEX idx = registeredFormats.FindFormat(format);
      if (idx != P_MAX_INDEX)
        OpalMediaFormatBaseList::Append(&registeredFormats[idx]);
    }
  }
  return *this;
}


OpalMediaFormatList & OpalMediaFormatList::operator-=(const OpalMediaFormat & format)
{
  PINDEX idx = FindFormat(format);
  if (idx != P_MAX_INDEX)
    RemoveAt(idx);

  return *this;
}


PINDEX OpalMediaFormatList::FindFormat(RTP_DataFrame::PayloadTypes pt) const
{
  for (PINDEX idx = 0; idx < GetSize(); idx++) {
    if ((*this)[idx].GetPayloadType() == pt)
      return idx;
  }

  return P_MAX_INDEX;
}


PINDEX OpalMediaFormatList::FindFormat(const PString & search) const
{
  PINDEX idx;
  PStringArray wildcards = search.Tokenise('*', TRUE);
  if (wildcards.GetSize() == 1) {
    for (idx = 0; idx < GetSize(); idx++) {
      if ((*this)[idx] == search)
        return idx;
    }
  }
  else {
    for (idx = 0; idx < GetSize(); idx++) {
      PCaselessString str = (*this)[idx];

      PINDEX i;
      PINDEX last = 0;
      for (i = 0; i < wildcards.GetSize(); i++) {
        PString wildcard = wildcards[i];

        PINDEX next;
        if (wildcard.IsEmpty())
          next = last;
        else {
          next = str.Find(wildcard, last);
          if (next == P_MAX_INDEX)
            break;
        }

        // Check for having * at beginning of search string
        if (i == 0 && next != 0 && !wildcard)
          break;

        last = next + wildcard.GetLength();

        // Check for having * at end of search string
        if (i == search.GetSize()-1 && !wildcard && last != str.GetLength())
          break;
      }

      if (i >= search.GetSize())
        return idx;
    }
  }

  return P_MAX_INDEX;
}


void OpalMediaFormatList::Reorder(const PStringArray & order)
{
  PINDEX nextPos = 0;
  for (PINDEX i =0; i < order.GetSize(); i++) {
    PINDEX findPos = FindFormat(order[i]);
    if (findPos != P_MAX_INDEX)
      OpalMediaFormatBaseList::InsertAt(nextPos++, RemoveAt(findPos));
  }
}


// End of File ///////////////////////////////////////////////////////////////
