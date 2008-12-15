/*
 * t140.cxx
 *
 * Implementation of T.140 Protocol for Multimedia Application Text Conversation
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision: 21293 $
 * $Author: rjongbloed $
 * $Date: 2008-10-13 10:24:41 +1100 (Mon, 13 Oct 2008) $
 */

#include <ptlib.h>
#include <opal/buildopts.h>

#ifdef __GNUC__
#pragma implementation "t140.h"
#endif

#include <im/t140.h>
#include <string.h>

#if OPAL_SIP
#include <sip/sdp.h>
#endif

#define ZERO_WIDTH_NO_BREAK 0xfeff
#define UTF_NEWLINE         0x2028

#if OPAL_SIP

/////////////////////////////////////////////////////////
//
//  SDP media description for audio media
//

class SDPT140MediaDescription : public SDPRTPAVPMediaDescription
{
  PCLASSINFO(SDPT140MediaDescription, SDPRTPAVPMediaDescription);
  public:
    SDPT140MediaDescription(const OpalTransportAddress & address)
      : SDPRTPAVPMediaDescription(address)
    { }

    virtual PString GetSDPMediaType() const
    {  return "text";  }
};

#endif

/////////////////////////////////////////////////////////////////////////////

OpalT140MediaType::OpalT140MediaType()
  : OpalRTPAVPMediaType("t140", "text", 7, false)
{
}

#if OPAL_SIP

SDPMediaDescription * OpalT140MediaType::CreateSDPMediaDescription(const OpalTransportAddress & localAddress)
{
  return new SDPT140MediaDescription(localAddress);
}

#endif

/////////////////////////////////////////////////////////////////////////////

T140String::T140String()
  : length(0)
{ 
  AppendUnicode16(ZERO_WIDTH_NO_BREAK); 
}

T140String::T140String(const PBYTEArray & bytes)
  : length(0)
{ 
  AppendUnicode16(ZERO_WIDTH_NO_BREAK);
  AppendUTF((const BYTE *)bytes, bytes.GetSize());
}

T140String::T140String(const char * chars)
  : length(0)
{ 
  AppendUnicode16(ZERO_WIDTH_NO_BREAK); 
  AppendUTF((const BYTE *)chars, strlen(chars));
}

T140String::T140String(const PString & str)
  : length(0)
{ 
  AppendUnicode16(ZERO_WIDTH_NO_BREAK); 
  AppendUTF((const BYTE *)(const char *)str, str.GetLength());
}

PINDEX T140String::AppendUTF(const BYTE * utf, PINDEX utfLen)
{
  memcpy(GetPointer(length+utfLen)+length, utf, utfLen);
  length += utfLen;
  return utfLen;
}

PINDEX T140String::GetUTFLen(WORD c)
{
  if (c <= 0x7f)
    return 1;
  if (c <= 0x7ff)
    return 2;
/*
  if (c <= 0xffff)
    return 3;
*/

  return 0;
}

PINDEX T140String::AppendUnicode16(WORD c)
{
  unsigned len = GetUTFLen(c);
  if (len == 0)
    return 0;
  SetUTF(GetPointer(length+len)+length, c);
  length += len;
  return c;
}


PINDEX T140String::SetUTF(BYTE * ptr, WORD c)
{
  BYTE ch = (BYTE)(c >> 8);
  BYTE cl = (BYTE)(c & 0xff);

  if (c <= 0x7f) {
    ptr[0] = (BYTE)c;
    return 1;
  }
  if (c <= 0x7ff) {
    ptr[0] = 0xc0 | (ch << 2) | (cl >> 6); 
    ptr[1] = 0x80 | (cl & 0x3f);
    return 2;
  }
#if 0
  if (c <= 0xffff) {
    ptr[0] = 0xe0 | (ch >> 4) | (cl >> 6); 
    ptr[1] = 0x80 | (ch << 2) | (cl >> 6); 
    ptr[2] = 0x80 | (cl & 0x3f);
    return 3;
  }
#endif

  return 0;
}
