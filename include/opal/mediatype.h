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
//  define the type used to hold the media type identifiers, i.e. "audio", "video", "h.224", "fax" etc
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
    static const char * UserInput();

    void PrintOn(ostream & strm) const { strm << (std::string &)*this; }

    OpalMediaTypeDefinition * GetDefinition() const;
    static OpalMediaTypeDefinition * GetDefinition(const OpalMediaType & key);

#if 0  // not yet enabled
    // used to deconflict default session IDs
    typedef std::map<unsigned, OpalMediaType> SessionIDToMediaTypeMap_T;
    static SessionIDToMediaTypeMap_T & GetSessionIDToMediaTypeMap();
#endif

#if OPAL_SIP
  public:
    static OpalMediaType GetMediaTypeFromSDP(const std::string & key);
    static PString       GetSDPFromFromMediaType(const OpalMediaType & type);
    static OpalMediaTypeDefinition * GetDefinitionFromSDP(const std::string & key);
#endif  // OPAL_SIP
};

ostream & operator << (ostream & strm, const OpalMediaType & mediaType);


////////////////////////////////////////////////////////////////////////////
//
//  this class defines the functions needed to work with the media type, i.e. 
//
class OpalRTPConnection;
class RTP_UDP;

#if OPAL_RTP_AGGREGATE
class PHandleAggregator;
#else
typedef void * PHandleAggregator;
#endif

#if OPAL_SIP
class SDPMediaDescription;
class OpalTransportAddress;
#endif

class OpalMediaTypeDefinition  {
  public:
    OpalMediaTypeDefinition(const char * sdpType, unsigned defaultSessionId = 0);
    virtual ~OpalMediaTypeDefinition() { }

#if OPAL_SIP
  public:
    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress) = 0;
    virtual std::string GetSDPType() const { return sdpType; }
  protected:
    std::string sdpType;
#endif

#if 0   /// disabled
    virtual bool IsMediaAutoStart(bool) const = 0;
    virtual unsigned GetPreferredSessionId() const { return defaultSessionId; }

    virtual bool UseDirectMediaPatch() const = 0;

    virtual RTP_UDP * CreateRTPSession(OpalRTPConnection & conn,
#if OPAL_RTP_AGGREGATE
                                       PHandleAggregator * agg,
#endif
                                       unsigned sessionID, bool remoteIsNAT) = 0;

  protected:  
    unsigned defaultSessionId;

#endif  // disabled

};

////////////////////////////////////////////////////////////////////////////
//
//  define the factory used for keeping track of OpalMediaTypeDefintions
//
typedef PFactory<OpalMediaTypeDefinition> OpalMediaTypeFactory;

#if 0 // disabled

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

#endif // disabled

////////////////////////////////////////////////////////////////////////////
//
//  define a macro for declaring a new OpalMediaTypeDefinition factory
//

#define OPAL_DECLARE_MEDIA_TYPE(type, cls) \
namespace OpalMediaTypeSpace { \
  static PFactory<OpalMediaTypeDefinition>::Worker<cls> static_##type##_##cls(#type, true); \
} \


template <const char * Type, const char * sdp>
class SimpleMediaType : public OpalMediaTypeDefinition
{
  public:
    SimpleMediaType()
      : OpalMediaTypeDefinition(Type, sdp)
    { }

    virtual ~SimpleMediaType()                     { }

#if OPAL_SIP
  public:
    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & ) { return NULL; }
#endif

#if 0 // disabled
    virtual bool IsMediaAutoStart(bool) const      { return true; }
    virtual unsigned GetPreferredSessionId() const { return defaultSessionId; }
    virtual bool UseDirectMediaPatch() const       { return false; }
    virtual RTP_UDP * CreateRTPSession(OpalRTPConnection & ,
#if OPAL_RTP_AGGREGATE
                                       PHandleAggregator * ,
#endif
                                       unsigned , bool ) { return NULL; }
#endif // disabled
};


#define OPAL_DEFINE_MEDIA_TYPE(type, sdp) \
namespace OpalMediaTypeSpace { \
  char type##_type_string[] = #type; \
  char type##_sdp_string[] = #sdp; \
  typedef SimpleMediaType<type##_type_string, type##_sdp_string> OpalMediaType_##type; \
  static PFactory<OpalMediaTypeDefinition>::Worker<OpalMediaType_##type> static_##type(#type, true); \
}; \

#define OPAL_DEFINE_MEDIA_TYPE_NO_SDP(type) OPAL_DEFINE_MEDIA_TYPE(type, "") 

////////////////////////////////////////////////////////////////////////////
//
//  common ancestor for audio and video OpalMediaTypeDefinitions
//

class OpalRTPAVPMediaType : public OpalMediaTypeDefinition {
  public:
    OpalRTPAVPMediaType(const char * sdpType, unsigned sessionID);

#if 0 // disabled

    RTP_UDP * CreateRTPSession(OpalRTPConnection & conn,
#if OPAL_RTP_AGGREGATE
                               PHandleAggregator * agg,
#endif
                               unsigned sessionID, bool remoteIsNAT);
    bool UseDirectMediaPatch() const;

#endif // disabled

};

#if OPAL_AUDIO

class OpalAudioMediaType : public OpalRTPAVPMediaType {
  public:
    OpalAudioMediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif

#if 0 // disabled
    virtual bool IsMediaAutoStart(bool) const;
#endif // disabled
};

#endif  // OPAL_AUDIO

#if OPAL_VIDEO

class OpalVideoMediaType : public OpalRTPAVPMediaType {
  public:
    OpalVideoMediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif

#if 0 // disabled
    bool IsMediaAutoStart(bool) const;
#endif // disabled
};

#endif // OPAL_VIDEO

#endif // __OPAL_MEDIATYPE_H
