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
OPAL_INSTANTIATE_MEDIATYPE(OpalPresentationVideoMediaDefinition);
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

OpalMediaType::AutoStartMap::AutoStartMap()
{
}


bool OpalMediaType::AutoStartMap::Add(const PString & stringOption)
{
  PWaitAndSignal m(m_mutex);

  // get autostart option as lines
  PStringArray lines = stringOption.Lines();
  for (PINDEX i = 0; i < lines.GetSize(); ++i) {
    PCaselessString mediaTypeName, modeName;
    if (lines[i].Split(':', mediaTypeName, modeName))
      Add(mediaTypeName, modeName);
    else
      Add(lines[i], "sendrecv");
  }

  return !empty();
}


bool OpalMediaType::AutoStartMap::Add(const PCaselessString & mediaTypeName, const PCaselessString & modeName)
{
  OpalMediaType mediaType(mediaTypeName);
  if (mediaType.GetDefinition() == NULL) {
    PTRACE(2, PTraceModule(), "Illegal/unknown AutoStart media type \"" << mediaTypeName << '"');
    return false;
  }

  OpalMediaType::AutoStartMode mode;
  if (modeName == "offer" || modeName == "inactive")
    mode = OpalMediaType::OfferInactive;
  else if (modeName == "recvonly")
    mode = OpalMediaType::Receive;
  else if (modeName == "sendonly")
    mode = OpalMediaType::Transmit;
  else if (modeName == "yes" || modeName == "true" || modeName == "1" || modeName == "sendrecv" || modeName == "recvsend")
    mode = OpalMediaType::ReceiveTransmit;
  else if (modeName == "no" || modeName == "false" || modeName == "0" || modeName == "no-offer")
    mode = OpalMediaType::DontOffer;
  else if (modeName == "exclusive") {
    OpalMediaTypeList types = OpalMediaType::GetList();
    for (OpalMediaTypeList::iterator it = types.begin(); it != types.end(); ++it)
      insert(value_type(*it, mediaType == it->c_str() ? OpalMediaType::ReceiveTransmit : OpalMediaType::DontOffer));
    return true;
  }
  else {
    PTRACE(2, PTraceModule(), "Illegal AutoStart mode \"" << modeName << '"');
    return false;
  }

  return insert(value_type(mediaType, mode)).second;
}


ostream & operator<<(ostream & strm, OpalMediaType::AutoStartMode mode)
{
  switch (mode.AsBits()) {
    case OpalMediaType::OfferInactive :
      return strm << "inactive";
    case OpalMediaType::Receive :
      return strm << "recvonly";
    case OpalMediaType::Transmit :
      return strm << "sendonly";
    case OpalMediaType::ReceiveTransmit :
      return strm << "sendrecv";
    default:
      return strm << "no-offer";
  }
}


OpalMediaType::AutoStartMode OpalMediaType::AutoStartMap::GetAutoStart(const OpalMediaType & mediaType) const
{
  PWaitAndSignal m(m_mutex);
  const_iterator it = find(mediaType);
  return it == end() ? mediaType.GetAutoStart() : it->second;
}


void OpalMediaType::AutoStartMap::SetGlobalAutoStart()
{
  for (iterator it = begin(); it != end(); ++it)
    it->first->SetAutoStart(it->second);
}


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


void OpalMediaTypeList::PrintOn(ostream & strm) const
{
  char f = strm.fill();
  for (const_iterator it = begin(); it != end(); ++it) {
    if (it != begin())
      strm << f;
    strm << *it;
  }
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


void OpalMediaTypeDefinition::SetAutoStart(OpalMediaType::AutoStartMode mode, bool on)
{
  if (on) {
    if (!(mode&OpalMediaType::DontOffer))
      m_autoStart -= OpalMediaType::DontOffer;
    m_autoStart += mode;
  }
  else
    m_autoStart -= mode;
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
{
}

OpalVideoMediaDefinition::OpalVideoMediaDefinition(const char * mediaType, unsigned defaultSessionId)
  : OpalRTPAVPMediaDefinition(mediaType, defaultSessionId)
{
}

const char * OpalPresentationVideoMediaDefinition::Name() { return "presentation"; }

OpalPresentationVideoMediaDefinition::OpalPresentationVideoMediaDefinition(const char * mediaType)
  : OpalVideoMediaDefinition(mediaType, 0)
{
}

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////
