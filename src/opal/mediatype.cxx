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
 * $Revision: 19424 $
 * $Author: csoutheren $
 * $Date: 2008-02-08 17:24:10 +1100 (Fri, 08 Feb 2008) $
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "mediatype.h"
#endif

#include <opal/buildopts.h>
#include <opal/mediatype.h>
#include <opal/rtpconn.h>
#include <opal/call.h>

#if OPAL_H323
#include <h323/channels.h>
#endif

namespace PWLibStupidLinkerHacks {
  int mediaTypeLoader;
}; // namespace PWLibStupidLinkerHacks

#if OPAL_AUDIO
OPAL_INSTANTIATE_MEDIATYPE(audio, OpalAudioMediaType);
#endif

#if OPAL_VIDEO
OPAL_INSTANTIATE_MEDIATYPE(video, OpalVideoMediaType);
#endif

OPAL_INSTANTIATE_SIMPLE_MEDIATYPE_NO_SDP(userinput); 

///////////////////////////////////////////////////////////////////////////////

ostream & operator << (ostream & strm, const OpalMediaType & mediaType)
{ mediaType.PrintOn(strm); return strm; }

const char * OpalMediaType::Audio()     { static const char * str = "audio";     return str; }
const char * OpalMediaType::Video()     { static const char * str = "video";     return str; }
const char * OpalMediaType::Fax()       { static const char * str = "fax";       return str; };
const char * OpalMediaType::UserInput() { static const char * str = "userinput"; return str; };

///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition * OpalMediaType::GetDefinition() const
{
  return OpalMediaTypeFactory::CreateInstance(*this);
}


OpalMediaTypeDefinition * OpalMediaType::GetDefinition(const OpalMediaType & key)
{
  return OpalMediaTypeFactory::CreateInstance(key);
}

///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition::OpalMediaTypeDefinition(const char * _mediaType, const char * _sdpType, unsigned preferredSessionId)
#if OPAL_SIP
  : sdpType(_sdpType != NULL ? _sdpType : "")
#endif
{
  PWaitAndSignal m(GetMapMutex());

  SessionIDToMediaTypeMap_T & typeMap = GetSessionIDToMediaTypeMap();

  // if the preferred session ID is already taken, then find a new one
  if (typeMap.find(preferredSessionId) != typeMap.end())
    preferredSessionId = 0;

  // if no preferred session Id 
  if (preferredSessionId == 0) {
    preferredSessionId = 1;
    while (typeMap.find(preferredSessionId) != typeMap.end())
      ++preferredSessionId;
  }

  PTRACE(1, "Opal\tMedia type " << _mediaType << " assigned session ID " << preferredSessionId);

  typeMap.insert(SessionIDToMediaTypeMap_T::value_type(preferredSessionId, _mediaType));
  GetMediaTypeToSessionIDMap().insert(MediaTypeToSessionIDMap_T::value_type(_mediaType, preferredSessionId));
}

unsigned OpalMediaTypeDefinition::GetDefaultSessionId(const OpalMediaType & mediaType)
{
  PWaitAndSignal m(GetMapMutex());



  MediaTypeToSessionIDMap_T::iterator r = GetMediaTypeToSessionIDMap().find(mediaType);
  if (r != GetMediaTypeToSessionIDMap().end())
    return r->second;

  return 0;
}


OpalMediaType OpalMediaTypeDefinition::GetMediaTypeForSessionId(unsigned sessionId)
{
  PWaitAndSignal m(GetMapMutex());

  SessionIDToMediaTypeMap_T::iterator r = GetSessionIDToMediaTypeMap().find(sessionId);
  if (r != GetSessionIDToMediaTypeMap().end())
    return r->second;

  return OpalMediaType();
}


OpalMediaTypeDefinition::SessionIDToMediaTypeMap_T & OpalMediaTypeDefinition::GetSessionIDToMediaTypeMap()
{
  PWaitAndSignal m(GetMapMutex());
  static SessionIDToMediaTypeMap_T map;
  return map;
}

OpalMediaTypeDefinition::MediaTypeToSessionIDMap_T & OpalMediaTypeDefinition::GetMediaTypeToSessionIDMap()
{
  PWaitAndSignal m(GetMapMutex());
  static MediaTypeToSessionIDMap_T map;
  return map;
}


PMutex & OpalMediaTypeDefinition::GetMapMutex()
{
  static PMutex mutex;
  return mutex;
}


RTP_UDP * OpalMediaTypeDefinition::CreateRTPSession(OpalRTPConnection & /*conn*/,
#if OPAL_RTP_AGGREGATE
                                                PHandleAggregator * agg,
#endif
                                                OpalSecurityMode * securityMode, unsigned sessionID, bool remoteIsNAT)
{
  if (securityMode != NULL) 
    return NULL;

  return new RTP_UDP(GetRTPEncoding(),
#if OPAL_RTP_AGGREGATE
        agg,
#endif
        sessionID, remoteIsNAT);
}

///////////////////////////////////////////////////////////////////////////////

OpalRTPAVPMediaType::OpalRTPAVPMediaType(const char * mediaType, const char * sdpType, unsigned preferredSessionId)
  : OpalMediaTypeDefinition(mediaType, sdpType, preferredSessionId)
{
}

PString OpalRTPAVPMediaType::GetRTPEncoding() const
{
  return "rtp/avp";
}

///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

OpalAudioMediaType::OpalAudioMediaType()
  : OpalRTPAVPMediaType("audio", "audio", 1)
{
}

#endif // OPAL_AUDIO

///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

OpalVideoMediaType::OpalVideoMediaType()
  : OpalRTPAVPMediaType("video", "video", 2)
{ }

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

