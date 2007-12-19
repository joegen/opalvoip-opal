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
extern int opalLoader;

static class InstantiateMe
{
  public:
    InstantiateMe()
    { 
      opalLoader = 1; 
    }
} instance;

}; // namespace PWLibStupidLinkerHacks

OPAL_DECLARE_MEDIA_TYPE(null, OpalNULLMediaType);

#if OPAL_AUDIO
OPAL_DECLARE_MEDIA_TYPE(audio, OpalAudioMediaType);
#endif

#if OPAL_VIDEO
OPAL_DECLARE_MEDIA_TYPE(video, OpalVideoMediaType);
#endif

///////////////////////////////////////////////////////////////////////////////

PBoolean OpalMediaTypeFactory::Contains(const OpalMediaType & mediaType)
{
  return OpalMediaTypeFactory::GetKeyMap().find(mediaType) != GetKeyMap().end();
};


OpalMediaTypeDefinition::OpalMediaTypeDefinition()
{ }

//PCaselessString OpalMediaTypeDefinition::GetTransport() const
//{ return GetDefaultTransport(); }


BYTE OpalMediaTypeDefinition::GetPreferredSessionId() const
{ return 0; }


PBoolean OpalMediaTypeDefinition::IsMediaBypassPossible() const
{ return FALSE; }


//const char * OpalMediaTypeDefinition::GetSDPType() const
//{ return NULL; }

///////////////////////////////////////////////////////////////////////////////

BYTE OpalNULLMediaType::GetPreferredSessionId() const
{ return 0; }

OpalMediaStream * OpalNULLMediaType::CreateMediaStream(OpalConnection &, const OpalMediaFormat &, const OpalMediaSessionId &, PBoolean)
{ return NULL; }

RTP_UDP * OpalNULLMediaType::CreateNonSecureSession(OpalConnection &, PHandleAggregator *, const OpalMediaSessionId &, PBoolean)
{ return NULL; }

H323Channel * OpalNULLMediaType::CreateH323Channel(H323Connection &, const H323Capability &, unsigned, RTP_Session &, const OpalMediaSessionId &, const H245_H2250LogicalChannelParameters *)
{
  return NULL;
}


///////////////////////////////////////////////////////////////////////////////

PBoolean OpalCommonMediaType::IsMediaBypassPossible() const
{ return TRUE; }

RTP_UDP * OpalCommonMediaType::CreateNonSecureSession(OpalConnection &, PHandleAggregator * aggregator, const OpalMediaSessionId & sessionID, PBoolean remoteIsNAT)
{  return new RTP_UDP(aggregator, sessionID, remoteIsNAT); }

OpalMediaStream * OpalCommonMediaType::CreateMediaStream(OpalConnection & connection, const OpalMediaFormat & mediaFormat, const OpalMediaSessionId & sessionID, PBoolean isSource)
{  
  if (connection.GetCall().IsMediaBypassPossible(connection, sessionID)) {
    PTRACE(3, "SIP\tBypassing media for session " << sessionID.sessionId);
    return new OpalNullMediaStream(connection, mediaFormat, sessionID, isSource);
  }

  RTP_Session * session = connection.GetSession(sessionID);
  if (session == NULL) {
    PTRACE(1, "H323\tCreateMediaStream could not find session " << sessionID.mediaType);
    return NULL;
  }

  return new OpalRTPMediaStream(connection, mediaFormat, isSource, *session,
                                connection.GetMinAudioJitterDelay(),
                                connection.GetMaxAudioJitterDelay());
}

#if OPAL_H323

H323Channel * OpalCommonMediaType::CreateH323Channel(H323Connection & conn, 
                                               const H323Capability & capability, 
                                                             unsigned direction, 
                                                        RTP_Session & session,
                                           const OpalMediaSessionId & /*sessionId*/,
                           const H245_H2250LogicalChannelParameters * /*param*/)
{  return new H323_RTPChannel(conn, capability, (H323Channel::Directions)direction, session); }

#endif // OPAL_H323



///////////////////////////////////////////////////////////////////////////////

#if OPAL_AUDIO

BYTE OpalAudioMediaType::GetPreferredSessionId() const
{ return 1; }

#endif // OPAL_AUDIO

///////////////////////////////////////////////////////////////////////////////

#if OPAL_VIDEO

BYTE OpalVideoMediaType::GetPreferredSessionId() const
{ return 2; }

OpalMediaStream * OpalVideoMediaType::CreateMediaStream(OpalConnection & conn, 
                                                        const OpalMediaFormat & mediaFormat,
                                                        const OpalMediaSessionId & sessionID,
                                                        PBoolean isSource)
{
  if (isSource) {
    PVideoInputDevice * videoDevice;
    PBoolean autoDelete;
    if (conn.CreateVideoInputDevice(mediaFormat, videoDevice, autoDelete)) {
      PVideoOutputDevice * previewDevice;
      if (!conn.CreateVideoOutputDevice(mediaFormat, PTrue, previewDevice, autoDelete))
        previewDevice = NULL;
      return new OpalVideoMediaStream(conn, mediaFormat, sessionID, videoDevice, previewDevice, autoDelete);
    }
  }
  else {
    PVideoOutputDevice * videoDevice;
    PBoolean autoDelete;
    if (conn.CreateVideoOutputDevice(mediaFormat, PFalse, videoDevice, autoDelete))
      return new OpalVideoMediaStream(conn, mediaFormat, sessionID, NULL, videoDevice, autoDelete);
  }

  return NULL;
}

#endif // OPAL_VIDEO

///////////////////////////////////////////////////////////////////////////////

OpalMediaSessionId::OpalMediaSessionId(const OpalMediaType & _mediaType, unsigned _sessionId)
  : mediaType(_mediaType), sessionId(_sessionId)
{
  if (sessionId == 0) {
    OpalMediaTypeDefinition * def = OpalMediaTypeFactory::CreateInstance(mediaType);
    if (def != NULL)
      sessionId = def->GetPreferredSessionId();
  }
}

