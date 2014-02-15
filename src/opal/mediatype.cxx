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

#include <opal_config.h>

#include <opal/mediatype.h>
#include <rtp/rtpconn.h>
#include <opal/call.h>
#include <opal/endpoint.h>
#include <opal/manager.h>

#if OPAL_H323
#include <h323/channels.h>
#endif

#include <algorithm>


OPAL_INSTANTIATE_MEDIATYPE(OpalAudioMediaDefinition);

#if OPAL_VIDEO
OPAL_INSTANTIATE_MEDIATYPE(OpalVideoMediaDefinition);
#endif

OPAL_INSTANTIATE_SIMPLE_MEDIATYPE(UserInputMediaDefinition, "userinput");


///////////////////////////////////////////////////////////////////////////////

const OpalMediaType & OpalMediaType::Audio()     { static const OpalMediaType type = OpalAudioMediaDefinition::Name(); return type; }
#if OPAL_VIDEO
const OpalMediaType & OpalMediaType::Video()     { static const OpalMediaType type = OpalVideoMediaDefinition::Name(); return type; }
#endif
#if OPAL_T38_CAPABILITY
const OpalMediaType & OpalMediaType::Fax()       { static const OpalMediaType type = OpalFaxMediaDefinition::Name();   return type; };
#endif
const OpalMediaType & OpalMediaType::UserInput() { static const OpalMediaType type = UserInputMediaDefinition::Name(); return type; };


///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition * OpalMediaType::GetDefinition() const
{
  return OpalMediaTypesFactory::CreateInstance(*this);
}


OpalMediaTypeDefinition * OpalMediaType::GetDefinition(const OpalMediaType & key)
{
  return OpalMediaTypesFactory::CreateInstance(key);
}


OpalMediaTypeList OpalMediaType::GetList()
{
  OpalMediaTypeList types = OpalMediaTypesFactory::GetKeyList();
  types.PrioritiseAudioVideo();
  return types;
}


void OpalMediaTypeList::PrioritiseAudioVideo()
{
  OpalMediaTypesFactory::KeyList_T::iterator it;

#if OPAL_VIDEO
  // For maximum compatibility, make sure audio/video are first
  it = std::find(begin(), end(), OpalMediaType::Video());
  if (it != end() && it != begin()) {
    erase(it);
    insert(begin(), OpalMediaType::Video());
  }
#endif

  it = std::find(begin(), end(), OpalMediaType::Audio());
  if (it != end() && it != begin()) {
    erase(it);
    insert(begin(), OpalMediaType::Audio());
  }
}


///////////////////////////////////////////////////////////////////////////////

OpalMediaTypeDefinition::OpalMediaTypeDefinition(const char * mediaType,
                                                 const char * mediaSession,
                                                 unsigned defaultSessionId,
                                                 OpalMediaType::AutoStartMode autoStart)
  : m_mediaType(mediaType)
  , m_mediaSessionType(mediaSession)
  , m_defaultSessionId(defaultSessionId)
  , m_autoStart(autoStart)
{
}


OpalMediaTypeDefinition::~OpalMediaTypeDefinition()
{
}


///////////////////////////////////////////////////////////////////////////////

OpalRTPAVPMediaDefinition::OpalRTPAVPMediaDefinition(const char * mediaType,
                                                     unsigned defaultSessionId,
                                                     OpalMediaType::AutoStartMode autoStart)
  : OpalMediaTypeDefinition(mediaType, OpalRTPSession::RTP_AVP(), defaultSessionId, autoStart)
{
}


///////////////////////////////////////////////////////////////////////////////

const char * OpalAudioMediaDefinition::Name() { return "audio"; }

OpalAudioMediaDefinition::OpalAudioMediaDefinition()
  : OpalRTPAVPMediaDefinition(Name(), 1, OpalMediaType::ReceiveTransmit)
{
}


///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

const char * OpalVideoMediaDefinition::Name() { return "video"; }

OpalVideoMediaDefinition::OpalVideoMediaDefinition()
  : OpalRTPAVPMediaDefinition(Name(), 2)
{ }

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////
