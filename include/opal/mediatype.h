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
 */

#ifndef OPAL_OPAL_MEDIATYPE_H
#define OPAL_OPAL_MEDIATYPE_H

#include <opal_config.h>

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
class OpalMediaTypeList : public OpalMediaTypesFactory::KeyList_T, public PObject
{
  public:
    OpalMediaTypeList() { }

    OpalMediaTypeList(const OpalMediaTypesFactory::KeyList_T & list)
      : OpalMediaTypesFactory::KeyList_T(list) { }

    virtual void PrintOn(ostream & strm) const;

    void PrioritiseAudioVideo();
};


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

    /** Get a list of all media types.
        This also assures that Audio() and Video() are the first two elements.
      */
    static OpalMediaTypeList GetList();

    P_DECLARE_BITWISE_ENUM_EX(AutoStartMode, 3,
                              (OfferInactive, Receive, Transmit, DontOffer),
                              ReceiveTransmit = Receive|Transmit,
                              TransmitReceive = Receive|Transmit);

    AutoStartMode GetAutoStart() const;

    class AutoStartMap : public std::map<OpalMediaType, OpalMediaType::AutoStartMode>
    {
      public:
        AutoStartMap();

        bool Add(const PString & stringOption);
        bool Add(const PCaselessString & mediaTypeName, const PCaselessString & modeName);

        OpalMediaType::AutoStartMode GetAutoStart(const OpalMediaType & mediaType) const;

        void SetGlobalAutoStart();

      protected:
        PDECLARE_MUTEX(m_mutex);
    };
};


__inline ostream & operator << (ostream & strm, const OpalMediaType & mediaType)
{
  return strm << mediaType.c_str();
}

ostream & operator<<(ostream & strm, OpalMediaType::AutoStartMode mode);


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
      unsigned defaultSessionId = 0,   ///< Session ID to use
      OpalMediaType::AutoStartMode autoStart = OpalMediaType::DontOffer   ///< Default value for auto-start transmit & receive
    );

    // Needed to avoid gcc warning about classes with virtual functions and 
    //  without a virtual destructor
    virtual ~OpalMediaTypeDefinition();

    // Get the media type instance
    const OpalMediaType & GetMediaType() const { return m_mediaType; }

    /** Get flags for media type can auto-start on call initiation.
      */
    OpalMediaType::AutoStartMode GetAutoStart() const { return m_autoStart; }

    /** Set flag for media type can auto-start receive on call initiation.
      */
    void SetAutoStart(OpalMediaType::AutoStartMode v) { m_autoStart = v; }
    void SetAutoStart(OpalMediaType::AutoStartMode v, bool on);

    /** Return the default session ID for this media type.
      */
    unsigned GetDefaultSessionId() const { return m_defaultSessionId; }

    /** Return the default session type (factory name) for this media type.
      */
    const PCaselessString & GetMediaSessionType() const { return m_mediaSessionType; }

#if OPAL_SDP
    virtual PString GetSDPMediaType() const;
    virtual PString GetSDPTransportType() const;

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
#endif // OPAL_SDP

  protected:
    OpalMediaType   m_mediaType;
    PCaselessString m_mediaSessionType;
    unsigned        m_defaultSessionId;
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

#define OPAL_MEDIATYPE(clsBase) \
  OPAL_INSTANTIATE_MEDIATYPE2(clsBase##Definition, clsBase##Definition::Name()); \
  const OpalMediaType & clsBase##Type() { static OpalMediaType t(clsBase##Definition::Name()); return t; }


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

class OpalRTPAVPMediaDefinition : public OpalMediaTypeDefinition
{
  public:
    OpalRTPAVPMediaDefinition(
      const char * mediaType, 
      unsigned     defaultSessionId = 0,
      OpalMediaType::AutoStartMode autoStart = OpalMediaType::DontOffer
    );

#if OPAL_SDP
    virtual bool MatchesSDP(const PCaselessString &, const PCaselessString &, const PStringArray &, PINDEX);
#endif
};


class OpalAudioMediaDefinition : public OpalRTPAVPMediaDefinition
{
  public:
    static const char * Name();

    OpalAudioMediaDefinition();

#if OPAL_SDP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress &) const;
#endif
};


#if OPAL_VIDEO

class OpalVideoMediaDefinition : public OpalRTPAVPMediaDefinition
{
  public:
    static const char * Name();

    OpalVideoMediaDefinition();

#if OPAL_SDP
    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress &) const;
#endif

  protected:
    OpalVideoMediaDefinition(const char * mediaType, unsigned defaultSessionId);
};

class OpalPresentationVideoMediaDefinition : public OpalVideoMediaDefinition
{
  public:
    static const char * Name();

    OpalPresentationVideoMediaDefinition(const char * mediaType = Name());
};
#endif // OPAL_VIDEO


#if OPAL_T38_CAPABILITY

class OpalFaxMediaDefinition : public OpalMediaTypeDefinition 
{
  public:
    static const char * Name();
    static const PCaselessString & UDPTL();

    OpalFaxMediaDefinition();

#if OPAL_SDP
    virtual PString GetSDPMediaType() const;
    virtual PString GetSDPTransportType() const;

    SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    ) const;
#endif // OPAL_SDP
};

#endif // OPAL_T38_CAPABILITY


__inline OpalMediaType::AutoStartMode OpalMediaType::GetAutoStart() const { return GetDefinition()->GetAutoStart(); }


#endif // OPAL_OPAL_MEDIATYPE_H
