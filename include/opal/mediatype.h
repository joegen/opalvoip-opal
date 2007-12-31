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
    OpalMediaType()
    { }

    virtual ~OpalMediaType()
    { }

    OpalMediaType(const std::string & _str)
      : std::string(_str) { }

    OpalMediaType(const char * _str)
      : std::string(_str) { }

    OpalMediaType(const PString & _str)
      : std::string((const char *)_str) { }

    static const char * Audio();
    static const char * Video();
    static const char * Fax();

    void PrintOn(ostream & strm) const { strm << (std::string &)*this; }

    OpalMediaTypeDefinition * GetDefinition() const;
    static OpalMediaTypeDefinition * GetDefinition(const OpalMediaType & key);

#if OPAL_SIP
  public:
    static OpalMediaType GetMediaTypeFromSDP(const std::string & key);
    static OpalMediaTypeDefinition * GetDefinitionFromSDP(const std::string & key);
    typedef std::map<std::string, OpalMediaType> SDPToMediaTypeMap_T;
    static SDPToMediaTypeMap_T & GetSDPToMediaTypeMap();
#endif  // OPAL_SIP

  public:
    // these functions/types will disappear once OpalMediaFormat uses OpalMediaType
    static OpalMediaTypeDefinition * GetDefinitionFromSessionID(unsigned sessionID);
    typedef std::map<unsigned, OpalMediaType> SessionIDToMediaTypeMap_T;
    static SessionIDToMediaTypeMap_T & GetSessionIDToMediaTypeMap();
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

#if OPAL_SIP
class SDPMediaDescription;
class OpalTransportAddress;
#endif

class OpalMediaTypeDefinition  {
  public:
    OpalMediaTypeDefinition(const char * mediaType, const char * sdpType, unsigned defaultSessionId);
    virtual ~OpalMediaTypeDefinition() { }

    virtual bool IsMediaAutoStart(bool) const = 0;
    virtual unsigned GetPreferredSessionId() const { return defaultSessionId; }

#if OPAL_SIP
    virtual std::string GetSDPType() const { return sdpType; }
    virtual PCaselessString GetTransport() const = 0;
    virtual SDPMediaDescription * CreateSDPMediaDescription(OpalTransportAddress & localAddress) = 0;
#endif
  protected:
    std::string sdpType;
#if OPAL_SIP
    unsigned defaultSessionId;
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
bool OpalMediaTypeIterate(Function & func)
{
  OpalMediaTypeFactory::KeyList_T keys = OpalMediaTypeFactory::GetKeyList();
  OpalMediaTypeFactory::KeyList_T::iterator r;
  for (r = keys.begin(); r != keys.end(); ++r) {
    if (!func(*r))
      break;
  }
  return r == keys.end();
}

template<class ObjType>
struct OpalMediaTypeIteratorObj
{
  typedef bool (ObjType::*ObjTypeFn)(const OpalMediaType &); 
  OpalMediaTypeIteratorObj(ObjType & _obj, ObjTypeFn _fn)
    : obj(_obj), fn(_fn) { }

  bool operator ()(const OpalMediaType & key)
  { return (obj.*fn)(key); }

  ObjType & obj;
  ObjTypeFn fn;
};


template<class ObjType, class Arg1Type>
struct OpalMediaTypeIteratorObj1Arg
{
  typedef bool (ObjType::*ObjTypeFn)(const OpalMediaType &, Arg1Type); 
  OpalMediaTypeIteratorObj1Arg(ObjType & _obj, Arg1Type _arg1, ObjTypeFn _fn)
    : obj(_obj), arg1(_arg1), fn(_fn) { }

  bool operator ()(const OpalMediaType & key)
  { return (obj.*fn)(key, arg1); }

  ObjType & obj;
  Arg1Type arg1;
  ObjTypeFn fn;
};

template<class ObjType, class Arg1Type, class Arg2Type>
struct OpalMediaTypeIteratorObj2Arg
{
  typedef bool (ObjType::*ObjTypeFn)(const OpalMediaType &, Arg1Type, Arg2Type); 
  OpalMediaTypeIteratorObj2Arg(ObjType & _obj, Arg1Type _arg1, Arg2Type _arg2, ObjTypeFn _fn)
    : obj(_obj), arg1(_arg1), arg2(_arg2), fn(_fn) { }

  bool operator ()(const OpalMediaType & key)
  { return (obj.*fn)(key, arg1, arg2); }

  ObjType & obj;
  Arg1Type arg1;
  Arg2Type arg2;
  ObjTypeFn fn;
};

template<class ObjType, class Arg1Type, class Arg2Type, class Arg3Type>
struct OpalMediaTypeIteratorObj3Arg
{
  typedef bool (ObjType::*ObjTypeFn)(const OpalMediaType &, Arg1Type, Arg2Type, Arg3Type); 
  OpalMediaTypeIteratorObj3Arg(ObjType & _obj, Arg1Type _arg1, Arg2Type _arg2, Arg3Type _arg3, ObjTypeFn _fn)
    : obj(_obj), arg1(_arg1), arg2(_arg2), arg3(_arg3), fn(_fn) { }

  bool operator ()(const OpalMediaType & key)
  { return (obj.*fn)(key, arg1, arg2, arg3); }

  ObjType & obj;
  Arg1Type arg1;
  Arg2Type arg2;
  Arg3Type arg3;
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
#if 0
class OpalNULLMediaType : public OpalMediaTypeDefinition {
  public:
    OpalNULLMediaType();

    bool IsMediaAutoStart(bool) const;

#if OPAL_SIP
    virtual PCaselessString GetTransport() const;
    SDPMediaDescription * CreateSDPMediaDescription(OpalTransportAddress &) { return NULL; }
#endif
};
#endif

////////////////////////////////////////////////////////////////////////////
//
//  common ancestor for audio and video OpalMediaTypeDefinitions
//

class OpalRTPAVPMediaType : public OpalMediaTypeDefinition {
  public:
    OpalRTPAVPMediaType(const char * mediaType, const char * sdpType, unsigned sessionID);

#if OPAL_SIP
    virtual PCaselessString GetTransport() const;
#endif
};

#if OPAL_AUDIO

class OpalAudioMediaType : public OpalRTPAVPMediaType {
  public:
    OpalAudioMediaType();
    virtual bool IsMediaAutoStart(bool) const;

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(OpalTransportAddress & localAddress);
#endif
};

#endif  // OPAL_AUDIO

#if OPAL_VIDEO

class OpalVideoMediaType : public OpalRTPAVPMediaType {
  public:
    OpalVideoMediaType();
    bool IsMediaAutoStart(bool) const;

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(OpalTransportAddress & localAddress);
#endif
};

#endif // OPAL_VIDEO

#endif // __OPAL_MEDIATYPE_H
