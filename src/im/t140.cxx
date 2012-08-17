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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifdef __GNUC__
#pragma implementation "t140.h"
#endif

#include <ptlib.h>
#include <opal/buildopts.h>

#include <im/t140.h>


/////////////////////////////////////////////////////////////////////////////

T140String::T140String()
  : length(0)
{ 
  AppendUnicode16(ZERO_WIDTH_NO_BREAK);
  SetAt(length, '\0');
}


T140String::T140String(const PBYTEArray & bytes)
  : length(0)
{ 
  WORD ch;
  if (bytes.GetSize() < 3 ||
      GetUTF((const BYTE *)bytes, bytes.GetSize(), ch) != 3 ||
      ch != ZERO_WIDTH_NO_BREAK)
    AppendUnicode16(ZERO_WIDTH_NO_BREAK);
  AppendUTF((const BYTE *)bytes, bytes.GetSize());
  SetAt(length, '\0');
}


T140String::T140String(const BYTE * data, PINDEX len)
  : length(0)
{ 
  WORD ch;
  if (len < 3 ||
      GetUTF(data, len, ch) != 3 ||
      ch != ZERO_WIDTH_NO_BREAK)
    AppendUnicode16(ZERO_WIDTH_NO_BREAK); 
  AppendUTF((const BYTE *)data, len);
  SetAt(length, '\0');
}


T140String::T140String(const char * chars)
  : length(0)
{ 
  WORD ch;
  if (strlen(chars) < 3 ||
      GetUTF((const BYTE *)chars, strlen(chars), ch) != 3 ||
      ch != ZERO_WIDTH_NO_BREAK)
    AppendUnicode16(ZERO_WIDTH_NO_BREAK); 
  AppendUTF((const BYTE *)chars, strlen(chars));
  SetAt(length, '\0');
}


T140String::T140String(const PString & str)
  : length(0)
{ 
  WORD ch;
  if (str.GetLength() < 3 ||
      GetUTF((const BYTE *)(const char *)str, str.GetLength(), ch) != 3 ||
      ch != ZERO_WIDTH_NO_BREAK)
    AppendUnicode16(ZERO_WIDTH_NO_BREAK); 
  AppendUTF((const BYTE *)(const char *)str, str.GetLength());
  SetAt(length, '\0');
}


PINDEX T140String::AppendUTF(const BYTE * utf, PINDEX utfLen)
{
  if (utfLen > 0) {
    memcpy(GetPointer(length+utfLen)+length, utf, utfLen);
    length += utfLen;
  }
  return utfLen;
}


PINDEX T140String::GetUTFLen(WORD c)
{
  if (c <= 0x7f)
    return 1;
  if (c <= 0x7ff)
    return 2;

  return 3;
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

  //if (c <= 0xffff) {
  ptr[0] = 0xe0 | (ch >> 4); 
  ptr[1] = 0x80 | ((ch & 0xf) << 2) | (cl >> 6); 
  ptr[2] = 0x80 | (cl & 0x3f);
  return 3;
}


PINDEX T140String::GetUTF(PINDEX pos, WORD & ch)
{
  return GetUTF(GetPointer()+pos, GetSize()-pos, ch);
}


PINDEX T140String::GetUTF(const BYTE * ptr, PINDEX len, WORD & ch)
{
  if (len < 1)
    return 0;

  // 0x00 .. 0x7f
  if (ptr[0] <= 0x7f) {
    ch = ptr[0];
    return 1;
  }

  if (ptr[0] <= 0xc1 || len < 2) 
    return 0;

  // 0x80 .. 0x7ff
  if (ptr[0] <= 0xdf) {
    ch = ((ptr[0] & 0x1f) << 6) | (ptr[1] & 0x3f);
    return 2;
  }

  if (ptr[0] > 0xef || len < 3) 
    return 0;

  // 0x800 .. 0xffff
  ch = ((ptr[0] & 0xf) << 12) | ((ptr[1] & 0x3f) << 6) | (ptr[2] & 0x3f);

  return 3;
}


bool T140String::AsString(PString & str)
{
  PINDEX pos = 0;
  while (pos < GetSize()) {
    WORD ch;
    PINDEX len = GetUTF(pos, ch);
    if (len == 0)
      return false;
    if (len == 1)
      str += (char)(ch & 0xff);
    else if (ch == UTF_NEWLINE)
      str += '\n';
    else // if (ch == ZERO_WIDTH_NO_BREAK) 
    {
      ;
    }
    pos += len;
  }

  return true;
}


/////////////////////////////////////////////////////////////////////////////

OpalT140RTPFrame::OpalT140RTPFrame(const RTP_DataFrame & frame)
  : RTP_DataFrame(frame)
{
}


OpalT140RTPFrame::OpalT140RTPFrame(const BYTE * data, PINDEX len, bool dynamic)
  : RTP_DataFrame(data, len, dynamic)
{
}


OpalT140RTPFrame::OpalT140RTPFrame()
{
  SetPayloadSize(0);
}


OpalT140RTPFrame::OpalT140RTPFrame(const PString & contentType)
{
  SetPayloadSize(0);
  SetContentType(contentType);
}


OpalT140RTPFrame::OpalT140RTPFrame(const PString & contentType, const T140String & content)
{
  SetPayloadSize(0);
  SetContentType(contentType);
  SetContent(content);
}


void OpalT140RTPFrame::SetContentType(const PString & contentType)
{
  SetHeaderExtension(0, contentType.GetLength(), (const BYTE *)(const char *)contentType, RTP_DataFrame::RFC3550);
}


PString OpalT140RTPFrame::GetContentType() const
{
  unsigned id;
  PINDEX length;
  BYTE * ptr = GetHeaderExtension(id, length);
  return ptr != NULL ? PString((const char *)ptr, length) : PString::Empty();
}


void OpalT140RTPFrame::SetContent(const T140String & text)
{
  SetPayloadSize(text.GetSize());
  memcpy(GetPayloadPtr(), (const BYTE *)text, text.GetSize());
}


bool OpalT140RTPFrame::GetContent(T140String & text) const
{
  if (GetPayloadSize() == 0) 
    return false;

  text = T140String((const BYTE *)GetPayloadPtr(), GetPayloadSize());
  return true;
}


bool OpalT140RTPFrame::GetContent(PString & str) const
{
  if (GetPayloadSize() == 0) 
    return false;

  T140String text((const BYTE *)GetPayloadPtr(), GetPayloadSize());
  return text.AsString(str);
}


//////////////////////////////////////////////////////////////////////////////////////////
