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

#include <opal/buildopts.h>

#include <ptlib/pfactory.h>
#include <ptlib/bitwise_enum.h>


#ifdef P_USE_PRAGMA
#pragma interface
#endif


class OpalMediaTypeDefinition;
class OpalConnection;

////////////////////////////////////////////////////////////////////////////
//
//  define the factory used for keeping track of OpalMediaTypeDefintions
//
class OpalMediaType;
typedef PFactory<OpalMediaTypeDefinition, OpalMediaType> OpalMediaTypesFactory;
typedef OpalMediaTypesFactory::KeyList_T OpalMediaTypeList;


/** Define the type used to hold the media type identifiers, i.e. "audio", "video", "h.224", "fax" etc
  */
class OpalMediaType : public std::string     // do not make this PCaselessString as that type does not work as index for std::map etc
{
  public:
    OpalMediaType()
    { }

    virtual ~OpalMediaType()
    { }

    OpalMediaType(const std::string & str)
      : std::string(str) { }

    OpalMediaType(const char * str)
      : std::string(str) { }

    OpalMediaType(const PString & str)
      : std::string((const char *)str) { }

    static const OpalMediaType & Audio();
#if OPAL_VIDEO
    static const OpalMediaType & Video();
#endif
#if OPAL_T38_CAPABILITY
    static const OpalMediaType & Fax();
#endif
    static const OpalMediaType & UserInput();

    OpalMediaTypeDefinition * operator->() const { return GetDefinition(); }
    OpalMediaTypeDefinition * GetDefinition() const;
    static OpalMediaTypeDefinition * GetDefinition(const OpalMediaType & key);
    static OpalMediaTypeDefinition * GetDefinition(unsigned sessionId);

    /** Get a list of all media types.
        This also assures that Audio() and Video() are the first two elements.
      */
    static OpalMediaTypeList GetList();

    P_DECLARE_BITWISE_ENUM_EX(AutoStartMode, 3,
                              (OfferInactive, Receive, Transmit, DontOffer),
                              ReceiveTransmit = Receive|Transmit,
                              TransmitReceive = Receive|Transmit);

    AutoStartMode GetAutoStart() const;
};


__inline ostream & operator << (ostream & strm, const OpalMediaType & mediaType)
{
  return strm << mediaType.c_str();
}


////////////////////////////////////////////////////////////////////////////
//
//  this class defines the functions needed to work with the media type, i.e. 
//

class SDPMediaDescription;
class OpalTransportAddress;
class OpalMediaSession;


/** This class defines the type used to define the attributes of a media type
 */
class OpalMediaTypeDefinition
{
  public:
    /// Create a new media type definition
    OpalMediaTypeDefinition(
      const char * mediaType,          ///< name of the media type (audio, video etc)
      const char * mediaSession,       ///< name of media session class (via factory)
      unsigned requiredSessionId = 0,  ///< Session ID to use, asserts if already in use
      OpalMediaType::AutoStartMode autoStart = OpalMediaType::DontOffer   ///< Default value for auto-start transmit & receive
    );

    // Needed to avoid gcc warning about classes with virtual functions and 
    //  without a virtual destructor
    virtual ~OpalMediaTypeDefinition();

    /** Get flags for media type can auto-start on call initiation.
      */
    OpalMediaType::AutoStartMode GetAutoStart() const { return m_autoStart; }

    /** Set flag for media type can auto-start receive on call initiation.
      */
    void SetAutoStart(OpalMediaType::AutoStartMode v) { m_autoStart = v; }
    void SetAutoStart(OpalMediaType::AutoStartMode v, bool on) { if (on) m_autoStart |= v; else m_autoStart -= v; }

    /** Return the default session ID for this media type.
      */
    unsigned GetDefaultSessionId() const { return m_defaultSessionId; }

    /** Return the default session type (factory name) for this media type.
      */
    const PString & GetMediaSessionType() const { return m_mediaSessionType; }

#if OPAL_SIP
    ///  Determine of this media type is valid for SDP m= section
    virtual bool MatchesSDP(
      const PCaselessString & sdpMediaType,
      const PCaselessString & sdpTransport,
      const PStringArray & sdpLines,
      PINDEX index
    );

    ///  create an SDP media description entry for this media type
    virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    ) const;
#endif // OPAL_SIP

  protected:
    OpalMediaType m_mediaType;
    PString       m_mediaSessionType;
    unsigned      m_defaultSessionId;
    OpalMediaType::AutoStartMode m_autoStart;

  private:
    P_REMOVE_VIRTUAL(OpalMediaSession *, CreateMediaSession(OpalConnection &, unsigned), NULL);
};


////////////////////////////////////////////////////////////////////////////
//
//  define a macro for declaring a new OpalMediaTypeDefinition factory
//

#define OPAL_INSTANTIATE_MEDIATYPE2(cls, name) \
  PFACTORY_CREATE(OpalMediaTypesFactory, cls, name, true)

#define OPAL_INSTANTIATE_MEDIATYPE(cls) \
  OPAL_INSTANTIATE_MEDIATYPE2(cls, cls::Name())


template <const char * Type, unsigned SessionId = 0>
class SimpleMediaType : public OpalMediaTypeDefinition
{
  public:
    static const char * Name() { return Type; }

    SimpleMediaType()
      : OpalMediaTypeDefinition(Name(), PString::Empty(), SessionId)
    { }
};

#define OPAL_INSTANTIATE_SIMPLE_MEDIATYPE(cls, name, ...) \
  namespace OpalMediaTypeSpace { extern const char cls[] = name; }; \
  typedef SimpleMediaType<OpalMediaTypeSpace::cls, ##__VA_ARGS__> cls; \
  OPAL_INSTANTIATE_MEDIATYPE(cls) \


////////////////////////////////////////////////////////////////////////////
//
//  common ancestor for audio and video OpalMediaTypeDefinitions
//

class OpalRTPAVPMediaType : public OpalMediaTypeDefinition {
  public:
    OpalRTPAVPMediaType(
      const char * mediaType, 
      unsigned     requiredSessionId = 0,
      OpalMediaType::AutoStartMode autoStart = OpalMediaType::DontOffer
    );

#if OPAL_SIP
    virtual bool MatchesSDP(const PCaselessString &, const PCaselessString &, const PStringArray &, PINDEX);
#endif
};


class OpalAudioMediaType : public OpalRTPAVPMediaType {
  public:
    static const char * Name();

    OpalAudioMediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress &) const;
#endif
};


#if OPAL_VIDEO

class OpalVideoMediaType : public OpalRTPAVPMediaType {
  public:
    static const char * Name();

    OpalVideoMediaType();

#if OPAL_SIP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress &) const;
#endif
};

#endif // OPAL_VIDEO


#if OPAL_T38_CAPABILITY

class OpalFaxMediaType : public OpalMediaTypeDefinition 
{
  public:
    static const char * Name();
    static const PCaselessString & UDPTL();

    OpalFaxMediaType();

#if OPAL_SIP
    static const PCaselessString & GetSDPMediaType();
    static const PString & GetSDPTransportType();

    virtual bool MatchesSDP(
      const PCaselessString & sdpMediaType,
      const PCaselessString & sdpTransport,
      const PStringArray & sdpLines,
      PINDEX index
    );

    SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    ) const;
#endif // OPAL_SIP
};

#endif // OPAL_T38_CAPABILITY


__inline OpalMediaType::AutoStartMode OpalMediaType::GetAutoStart() const { return GetDefinition()->GetAutoStart(); }


#endif // OPAL_OPAL_MEDIATYPE_H
