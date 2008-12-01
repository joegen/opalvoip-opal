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

#ifndef OPAL_OPAL_MEDIATYPE_H
#define OPAL_OPAL_MEDIATYPE_H

#include <ptbuildopts.h>
#include <ptlib/pfactory.h>
#include <opal/buildopts.h>

#ifdef P_USE_PRAGMA
#pragma interface
#endif

namespace PWLibStupidLinkerHacks {
extern int mediaTypeLoader;
}; // namespace PWLibStupidLinkerHacks

class OpalMediaTypeDefinition;
class OpalSecurityMode;
class OpalConnection;

////////////////////////////////////////////////////////////////////////////
//
//  define the factory used for keeping track of OpalMediaTypeDefintions
//
typedef PFactory<OpalMediaTypeDefinition> OpalMediaTypeFactory;
typedef OpalMediaTypeFactory::KeyList_T OpalMediaTypeList;

////////////////////////////////////////////////////////////////////////////
//
//  define the type used to hold the media type identifiers, i.e. "audio", "video", "h.224", "fax" etc
//

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

    static const OpalMediaType & Audio();
    static const OpalMediaType & Video();
    static const OpalMediaType & Fax();
    static const OpalMediaType & UserInput();

    void PrintOn(ostream & strm) const { strm << c_str(); }

    OpalMediaTypeDefinition * GetDefinition() const;
    static OpalMediaTypeDefinition * GetDefinition(const OpalMediaType & key);

    static OpalMediaTypeFactory::KeyList_T GetList() { return OpalMediaTypeFactory::GetKeyList(); }

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

#if OPAL_SIP
class SDPMediaDescription;
class OpalTransportAddress;
#endif

class OpalMediaSession;

////////////////////////////////////////////////////////////////////////////
//
//  this class defines the type used to define the attributes of a media type
//

class OpalMediaTypeDefinition  {
  public:
    //
    //  create a new media type definition
    //
    OpalMediaTypeDefinition(
      const char * mediaType,          // name of the media type (audio, video etc)
      const char * sdpType,            // name of the SDP type 
      unsigned preferredSessionId      // preferred session ID
    );

    //
    //  needed to avoid gcc warning about classes with virtual functions and 
    //  without a virtual destructor
    //
    virtual ~OpalMediaTypeDefinition() { }

    //
    //  if true, then this type uses RTP for transport
    //  if not, then it uses a generic OpaMediaSession
    //
    virtual bool UsesRTP() const { return true; }

    //
    //
    //
    virtual OpalMediaSession * CreateMediaSession(OpalConnection & conn, unsigned sessionID) const;

    //
    //  get the string used for the RTP_FormatHandler PFactory which is used
    //  to create the RTP handler for the this media type
    //  possible values include "rtp/avp" and "udptl"
    //
    //  Only valid if UsesRTP return true
    //
    virtual PString GetRTPEncoding() const = 0;

    //
    //  create an RTP session for this media format
    //  By default, this will create a RTP_UDP session with the correct initial format
    //
    //  Only valid if UsesRTP return true
    //
    virtual RTP_UDP * CreateRTPSession(OpalRTPConnection & conn,
                                                  unsigned sessionID, 
                                                      bool remoteIsNAT);

    //
    // return the default session ID for a media type
    //
    unsigned GetDefaultSessionId()   { return GetDefaultSessionId(mediaType); }

    static unsigned GetDefaultSessionId(
      const OpalMediaType & mediaType
    );

    //
    // return the media type associated with a default media type
    //
    static OpalMediaType GetMediaTypeForSessionId(
      unsigned sessionId
    );

  protected:
    static PMutex & GetMapMutex();

    typedef std::map<OpalMediaType, unsigned> MediaTypeToSessionIDMap_T;
    static MediaTypeToSessionIDMap_T & GetMediaTypeToSessionIDMap();

    typedef std::map<unsigned, OpalMediaType> SessionIDToMediaTypeMap_T;
    static SessionIDToMediaTypeMap_T & GetSessionIDToMediaTypeMap();

    std::string mediaType;

#if OPAL_SIP
  public:
    //
    //  return the SDP type for this media type
    //
    virtual std::string GetSDPType() const 
    { return sdpType; }

    //
    //  create an SDP media description entry for this media type
    //
    virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    ) = 0;

  protected:
    std::string sdpType;
#endif
};


////////////////////////////////////////////////////////////////////////////
//
//  define a macro for declaring a new OpalMediaTypeDefinition factory
//

#define OPAL_INSTANTIATE_MEDIATYPE(type, cls) \
namespace OpalMediaTypeSpace { \
  static PFactory<OpalMediaTypeDefinition>::Worker<cls> static_##type##_##cls(#type, true); \
}; \


#ifdef SOLARIS
template <char * Type, char * sdp>
#else
template <char * Type, const char * sdp>
#endif
class SimpleMediaType : public OpalMediaTypeDefinition
{
  public:
    SimpleMediaType()
      : OpalMediaTypeDefinition(Type, sdp, 0)
    { }

    virtual ~SimpleMediaType()                     { }

    virtual RTP_UDP * CreateRTPSession(OpalRTPConnection & ,unsigned , bool ) { return NULL; }

    PString GetRTPEncoding() const { return PString::Empty(); } 

#if OPAL_SIP
  public:
    virtual SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & ) { return NULL; }
#endif
};


#define OPAL_INSTANTIATE_SIMPLE_MEDIATYPE(type, sdp) \
namespace OpalMediaTypeSpace { \
  char type##_type_string[] = #type; \
  char type##_sdp_string[] = #sdp; \
  typedef SimpleMediaType<type##_type_string, type##_sdp_string> type##_MediaType; \
}; \
OPAL_INSTANTIATE_MEDIATYPE(type, type##_MediaType) \

#define OPAL_INSTANTIATE_SIMPLE_MEDIATYPE_NO_SDP(type) OPAL_INSTANTIATE_SIMPLE_MEDIATYPE(type, "") 

////////////////////////////////////////////////////////////////////////////
//
//  common ancestor for audio and video OpalMediaTypeDefinitions
//

class OpalRTPAVPMediaType : public OpalMediaTypeDefinition {
  public:
    OpalRTPAVPMediaType(
      const char * mediaType, 
      const char * sdpType, 
          unsigned preferredSessionId
    );

    virtual PString GetRTPEncoding() const;

    OpalMediaSession * CreateMediaSession(OpalConnection & /*conn*/, unsigned /* sessionID*/) const;
};


class OpalAudioMediaType : public OpalRTPAVPMediaType {
  public:
    OpalAudioMediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};


#if OPAL_VIDEO

class OpalVideoMediaType : public OpalRTPAVPMediaType {
  public:
    OpalVideoMediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};

#endif // OPAL_VIDEO


#if OPAL_T38_CAPABILITY

class OpalFaxMediaType : public OpalMediaTypeDefinition 
{
  public:
    OpalFaxMediaType();

    PString GetRTPEncoding(void) const;
    RTP_UDP * CreateRTPSession(OpalRTPConnection & conn,
                               unsigned sessionID, bool remoteIsNAT);

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};

#endif // OPAL_T38_CAPABILITY


#if OPAL_IM_CAPABILITY

class OpalIMMediaType : public OpalMediaTypeDefinition 
{
  public:
    OpalIMMediaType();

    PString GetRTPEncoding(void) const;
    RTP_UDP * CreateRTPSession(OpalRTPConnection & conn, unsigned sessionID, bool remoteIsNAT);

    virtual bool UsesRTP() const;
    virtual OpalMediaSession * CreateMediaSession(OpalConnection & conn, unsigned sessionID) const;

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
#endif
};

#endif // OPAL_IM_CAPABILITY

#endif // OPAL_OPAL_MEDIATYPE_H
