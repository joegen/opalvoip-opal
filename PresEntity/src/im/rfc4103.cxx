/*
 * rfc4103.cxx
 *
 * Implementation of RFC 4103 RTP Payload for Text Conversation
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
#pragma implementation "rfc4103.h"
#endif

#include <ptlib.h>
#include <opal/buildopts.h>

#include <im/rfc4103.h>

#if OPAL_HAS_RFC4103

#include <sip/sdp.h>

/////////////////////////////////////////////////////////////////////////////

RFC4103Context::RFC4103Context(const OpalMediaFormat & fmt)
  : m_mediaFormat(fmt)
  , m_sequence(0)
  , m_baseTimeStamp(0)
{
}

RTP_DataFrameList RFC4103Context::ConvertToFrames(const T140String & body)
{
  DWORD ts = m_baseTimeStamp;
  ts += (DWORD)((PTime() - m_baseTime).GetMilliSeconds());

  RTP_DataFrameList frames;
  RFC4103Frame * frame = new RFC4103Frame();

  frame->SetPayloadType(m_mediaFormat.GetPayloadType());
  frame->SetMarker(true);
  frame->SetTimestamp(ts);
  frame->SetSequenceNumber(++m_sequence);
  frame->SetPayload(body);

  frames.Append(frame);

  return frames;
}

/////////////////////////////////////////////////////////////////////////////

RFC4103Frame::RFC4103Frame()
  : RTP_DataFrame(0)
{ 
}

RFC4103Frame::RFC4103Frame(const T140String & t140)
  : RTP_DataFrame(0)
{
  SetPayload(t140);
}

void RFC4103Frame::SetPayload(const T140String & t140)
{
  SetPayloadSize(t140.GetSize());
  memcpy(GetPayloadPtr(), (const BYTE *)t140, t140.GetLength());
}

#endif // OPAL_HAS_RFC4103
