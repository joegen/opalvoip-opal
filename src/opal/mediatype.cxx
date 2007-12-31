/*
 * mediatype.h
 *
 * Media Format Type descriptions
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment and Hannes Friederich
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
 * The Original Code is OPAL
 *
 * The Initial Developer of the Original Code is Hannes Friederich and Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediatype.h"
#endif

#include <opal/buildopts.h>
#include <opal/mediatype.h>
#include <opal/connection.h>
#include <opal/call.h>

#if OPAL_H323
#include <h323/channels.h>
#endif

namespace PWLibStupidLinkerHacks {
  int mediaTypeLoader;
}; // namespace PWLibStupidLinkerHacks


//OPAL_DECLARE_MEDIA_TYPE(null, OpalNULLMediaType);

#if OPAL_AUDIO
OPAL_DECLARE_MEDIA_TYPE(audio, OpalAudioMediaType);
#endif

#if OPAL_VIDEO
OPAL_DECLARE_MEDIA_TYPE(video, OpalVideoMediaType);
#endif


typedef std::map<std::string, std::string> MediaTypeToSDPMap_T;

///////////////////////////////////////////////////////////////////////////////

ostream & operator << (ostream & strm, const OpalMediaType & mediaType)
{ mediaType.PrintOn(strm); return strm; }

const char * OpalMediaType::Audio()  { static const char * str = "audio"; return str; }
const char * OpalMediaType::Video()  { static const char * str = "video"; return str; }
const char * OpalMediaType::Fax()    { static const char * str = "fax";   return str; };

///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition * OpalMediaType::GetDefinition() const
{
  return OpalMediaTypeFactory::CreateInstance(*this);
}

OpalMediaTypeDefinition * OpalMediaType::GetDefinition(const OpalMediaType & key)
{
  return OpalMediaTypeFactory::CreateInstance(key);
}


OpalMediaType::SessionIDToMediaTypeMap_T & OpalMediaType::GetSessionIDToMediaTypeMap()
{
  static SessionIDToMediaTypeMap_T sessionIDToMediaTypeMap;
  return sessionIDToMediaTypeMap;
}

OpalMediaTypeDefinition * OpalMediaType::GetDefinitionFromSessionID(unsigned sessionID)
{
  SessionIDToMediaTypeMap_T & map = GetSessionIDToMediaTypeMap();
  SessionIDToMediaTypeMap_T::iterator r = map.find(sessionID);
  if (r == map.end())
    return NULL;
  return GetDefinition(r->second);
}

///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition::OpalMediaTypeDefinition(const char * mediaType, const char * _sdpType, unsigned _defaultSessionId)
  : defaultSessionId(_defaultSessionId)
#if OPAL_SIP
  , sdpType(_sdpType)
#endif
{
#if OPAL_SIP
  OpalMediaType::GetSDPToMediaTypeMap().insert(OpalMediaType::SDPToMediaTypeMap_T::value_type(sdpType, mediaType));
#endif
  OpalMediaType::GetSessionIDToMediaTypeMap().insert(OpalMediaType::SessionIDToMediaTypeMap_T::value_type(defaultSessionId, mediaType));
}

///////////////////////////////////////////////////////////////////////////////

#if 0

OpalNULLMediaType::OpalNULLMediaType()
  : OpalMediaTypeDefinition("null", "")
{
}

bool OpalNULLMediaType::IsMediaAutoStart(bool) const 
{ return false; }

#endif

///////////////////////////////////////////////////////////////////////////////

OpalRTPAVPMediaType::OpalRTPAVPMediaType(const char * mediaType, const char * sdpType, unsigned sessionID)
  : OpalMediaTypeDefinition(mediaType, sdpType, sessionID)
{
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

OpalAudioMediaType::OpalAudioMediaType()
  : OpalRTPAVPMediaType("audio", "audio", 1)
{
}

bool OpalAudioMediaType::IsMediaAutoStart(bool) const 
{ return true; }

#endif // OPAL_AUDIO

///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMediaType::OpalVideoMediaType()
  : OpalRTPAVPMediaType("video", "video", 2)
{ }

bool OpalVideoMediaType::IsMediaAutoStart(bool) const 
{ return true; }

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

#if 0

OpalMediaSessionId::OpalMediaSessionId(const OpalMediaType & _mediaType, unsigned _sessionId)
  : mediaType(_mediaType), sessionId(_sessionId)
{
  if (sessionId == 0) {
    OpalMediaTypeDefinition * def = OpalMediaTypeFactory::CreateInstance(mediaType);
    if (def != NULL)
      sessionId = def->GetPreferredSessionId();
  }
}

#endif

