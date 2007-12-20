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

#ifndef __OPAL_MEDIATYPE_H
#define __OPAL_MEDIATYPE_H

#include <ptbuildopts.h>
#include <ptlib/pfactory.h>
#include <opal/buildopts.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif

namespace PWLibStupidLinkerHacks {
extern int mediaTypeLoader;
}; // namespace PWLibStupidLinkerHacks

////////////////////////////////////////////////////////////////////////////
//
//  define the type used to hold the media type identifiers, i.e. "audio", "video", "h.224", "t.38" etc
//

class OpalMediaTypeDefinition;

class OpalMediaType : public std::string     // do not make this PCaselessString as that type does not work as index for std::map etc
{
  public:
    OpalMediaType(const std::string & _str)
      : std::string(_str) { }

    OpalMediaType(const char * _str)
      : std::string(_str) { }

    OpalMediaType(const PString & _str)
      : std::string((const char *)_str) { }

    static const char * Audio();
    static const char * Video();
    static const char * Fax();

    OpalMediaTypeDefinition * GetDefinition() const;
    static OpalMediaTypeDefinition * GetDefinition(const OpalMediaType & key);

    void PrintOn(ostream & strm) const { strm << (std::string &)*this; }
};

ostream & operator << (ostream & strm, const OpalMediaType & mediaType);

////////////////////////////////////////////////////////////////////////////
//
//  define a class for holding a session ID and media type
//
#if 0

class OpalMediaSessionId 
{
  public:
    OpalMediaSessionId(const OpalMediaType & _mediaType, unsigned id = 0);

    OpalMediaType mediaType;
    unsigned int sessionId;
};

#endif

////////////////////////////////////////////////////////////////////////////
//
//  this class defines the functions needed to work with the media type, i.e. 
//

#if 0

#if OPAL_RTP_AGGREGATE
#include <ptclib/sockagg.h>
#else
typedef void * PHandleAggregator;
typedef void * RTP_AggregatedHandle;
#endif

class RTP_UDP;
class OpalConnection;
class OpalMediaStream;
class OpalMediaFormat;

#if OPAL_H323
class H323Capability;
class H323Connection;
class RTP_Session;
class H323Channel;
class H245_H2250LogicalChannelParameters;
#endif

#endif

#if OPAL_SIP
class SDPMediaDescription;
class OpalTransportAddress;
#endif

class OpalMediaTypeDefinition  {
  public:
    OpalMediaTypeDefinition();

    virtual PCaselessString GetTransport() const;
#if OPAL_SIP
    virtual SDPMediaDescription * CreateSDPMediaDescription(OpalTransportAddress & localAddress) = 0;
#endif

#if 0
    virtual PBoolean IsMediaBypassPossible() const;
    virtual BYTE GetPreferredSessionId() const;
    virtual bool IsMediaAutoStart(bool) const { return false; }

    virtual RTP_UDP * CreateNonSecureSession(OpalConnection & conn, 
                                          PHandleAggregator * aggregator, 
                                   const OpalMediaSessionId & id, 
                                                     PBoolean remoteIsNAT) = 0;

    virtual OpalMediaStream * CreateMediaStream(OpalConnection & conn, 
                                         const OpalMediaFormat & mediaFormat,
                                      const OpalMediaSessionId & sessionID,
                                                        PBoolean isSource) = 0;

#if OPAL_H323
    virtual H323Channel * CreateH323Channel(H323Connection & conn, 
                                      const H323Capability & capability, 
                                                    unsigned direction, 
                                               RTP_Session & session,
                                  const OpalMediaSessionId & sessionId,
                  const H245_H2250LogicalChannelParameters * param) = 0;
#endif
#endif

};

////////////////////////////////////////////////////////////////////////////
//
//  define the factory used for keeping track of OpalMediaTypeDefintions
//
typedef PFactory<OpalMediaTypeDefinition> OpalMediaTypeFactory;

////////////////////////////////////////////////////////////////////////////
//
//  define a useful template for iterating over all OpalMediaTypeDefintions
//
template<class Function>
void OpalMediaTypeIterate(Function & func)
{
  OpalMediaTypeFactory::KeyList_T keys = OpalMediaTypeFactory::GetKeyList();
  OpalMediaTypeFactory::KeyList_T::iterator r;
  for (r = keys.begin(); r != keys.end(); ++r) {
    func(*r);
  }
}

template<class ObjType>
struct OpalMediaTypeIteratorObj
{
  typedef void (ObjType::*ObjTypeFn)(const OpalMediaType &); 
  OpalMediaTypeIteratorObj(ObjType & _obj, ObjTypeFn _fn)
    : obj(_obj), fn(_fn) { }

  void operator ()(const OpalMediaType & key)
  { (obj.*fn)(key); }

  ObjType & obj;
  ObjTypeFn fn;
};


template<class ObjType, class Arg1Type>
struct OpalMediaTypeIteratorObj1Arg
{
  typedef void (ObjType::*ObjTypeFn)(const OpalMediaType &, Arg1Type &); 
  OpalMediaTypeIteratorObj1Arg(ObjType & _obj, Arg1Type & _arg1, ObjTypeFn _fn)
    : obj(_obj), arg1(_arg1), fn(_fn) { }

  void operator ()(const OpalMediaType & key)
  { (obj.*fn)(key, arg1); }

  ObjType & obj;
  Arg1Type & arg1;
  ObjTypeFn fn;
};

////////////////////////////////////////////////////////////////////////////
//
//  define a macro for declaring a new OpalMediaTypeDefinition factory
//

#define OPAL_DECLARE_MEDIA_TYPE(type, cls) \
  static PFactory<OpalMediaTypeDefinition>::Worker<cls> static_##type##_##cls(#type, true);


////////////////////////////////////////////////////////////////////////////
//
//  empty media type
//

class OpalNULLMediaType : public OpalMediaTypeDefinition {
  public:
#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(OpalTransportAddress &) { return NULL; }
#endif
#if 0
    PBoolean IsMediaBypassPossible() const  { return false; }
    BYTE GetPreferredSessionId() const;

    RTP_UDP * CreateNonSecureSession(OpalConnection & conn, PHandleAggregator * aggregator, const OpalMediaSessionId & id, PBoolean remoteIsNAT);
    OpalMediaStream * CreateMediaStream(OpalConnection & conn, const OpalMediaFormat & mediaFormat,const OpalMediaSessionId & sessionID, PBoolean isSource);

#if OPAL_H323
    virtual H323Channel * CreateH323Channel(H323Connection & conn, 
                                      const H323Capability & capability, 
                                                    unsigned direction, 
                                               RTP_Session & session,
                                  const OpalMediaSessionId & sessionId,
                  const H245_H2250LogicalChannelParameters * param);
#endif
#endif
};

////////////////////////////////////////////////////////////////////////////
//
//  common ancestor for audio and video OpalMediaTypeDefinitions
//

class OpalCommonMediaType : public OpalMediaTypeDefinition {
  public:
#if 0
    PBoolean IsMediaBypassPossible() const;
    virtual bool IsMediaAutoStart(bool) const { return true; }
    RTP_UDP * CreateNonSecureSession(OpalConnection & conn, PHandleAggregator * aggregator, const OpalMediaSessionId & id, PBoolean remoteIsNAT);
    OpalMediaStream * CreateMediaStream(OpalConnection & conn, const OpalMediaFormat & mediaFormat,const OpalMediaSessionId & sessionID, PBoolean isSource);
#if OPAL_H323
    virtual H323Channel * CreateH323Channel(H323Connection & conn, 
                                      const H323Capability & capability, 
                                                    unsigned direction, 
                                               RTP_Session & session,
                                  const OpalMediaSessionId & sessionId,
                  const H245_H2250LogicalChannelParameters * param);
#endif
#endif
};

#if OPAL_AUDIO

class OpalAudioMediaType : public OpalCommonMediaType {
  public:
#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(OpalTransportAddress & localAddress);
#endif
#if 0 
    BYTE GetPreferredSessionId() const;
#endif
};

#endif  // OPAL_AUDIO

#if OPAL_VIDEO

class OpalVideoMediaType : public OpalCommonMediaType {
  public:
#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(OpalTransportAddress & localAddress);
#endif
#if 0
  public:
    BYTE GetPreferredSessionId() const;
    OpalMediaStream * CreateMediaStream(OpalConnection & conn, const OpalMediaFormat & mediaFormat,const OpalMediaSessionId & sessionID, PBoolean isSource);
#endif
};

#endif // OPAL_VIDEO

#endif // __OPAL_MEDIATYPE_H
