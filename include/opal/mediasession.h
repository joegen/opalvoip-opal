/*
 * mediasession.h
 *
 * Media session abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2010 Vox Lucida Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Vox Lucida Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_OPAL_MEDIASESSION_H
#define OPAL_OPAL_MEDIASESSION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/transports.h>
#include <opal/mediatype.h>


class OpalConnection;
class OpalMediaStream;
class OpalMediaFormat;
class OpalMediaFormatList;
class OpalMediaCryptoSuite;


#if OPAL_STATISTICS

/**This class carries statistics on the media stream.
  */
class OpalMediaStatistics : public PObject
{
    PCLASSINFO(OpalMediaStatistics, PObject);
  public:
    OpalMediaStatistics();

    // General info (typicallly from RTP)
    PTime    m_startTime;
    PUInt64  m_totalBytes;
    unsigned m_totalPackets;
    unsigned m_packetsLost;
    unsigned m_packetsOutOfOrder;
    unsigned m_packetsTooLate;
    unsigned m_packetOverruns;
    unsigned m_minimumPacketTime;
    unsigned m_averagePacketTime;
    unsigned m_maximumPacketTime;

    // Audio
    unsigned m_averageJitter;
    unsigned m_maximumJitter;
    unsigned m_jitterBufferDelay;

#if OPAL_VIDEO
    // Video
    unsigned m_totalFrames;
    unsigned m_keyFrames;
    int      m_quality; // -1 is none, 0 is very good > 0 is progressively worse
#endif

    // Fax
#if OPAL_FAX
    enum {
      FaxNotStarted = -2,
      FaxInProgress = -1,
      FaxSuccessful = 0,
      FaxErrorBase  = 1
    };
    enum FaxCompression {
      FaxCompressionUnknown,
      FaxCompressionT4_1d,
      FaxCompressionT4_2d,
      FaxCompressionT6,
    };
    friend ostream & operator<<(ostream & strm, FaxCompression compression);
    struct Fax {
      Fax();

      int  m_result;      // -2=not started, -1=progress, 0=success, >0=ended with error
      char m_phase;       // 'A', 'B', 'D'
      int  m_bitRate;     // e.g. 14400, 9600
      FaxCompression m_compression; // 0=N/A, 1=T.4 1d, 2=T.4 2d, 3=T.6
      bool m_errorCorrection;
      int  m_txPages;
      int  m_rxPages;
      int  m_totalPages;
      int  m_imageSize;   // In bytes
      int  m_resolutionX; // Pixels per inch
      int  m_resolutionY; // Pixels per inch
      int  m_pageWidth;
      int  m_pageHeight;
      int  m_badRows;     // Total number of bad rows
      int  m_mostBadRows; // Longest run of bad rows
      int  m_errorCorrectionRetries;

      PString m_stationId; // Remote station identifier
      PString m_errorText;
    } m_fax;
#endif
};

#endif


/** Class for contianing the cryptographic keys for use by OpalMediaCryptoSuite.
  */
class OpalMediaCryptoKeyInfo : public PObject {
  protected:
    OpalMediaCryptoKeyInfo(const OpalMediaCryptoSuite & cryptoSuite) : m_cryptoSuite(cryptoSuite) { }

  public:
    virtual ~OpalMediaCryptoKeyInfo() { }

    virtual bool IsValid() const = 0;
    virtual void Randomise() = 0;
    virtual bool FromString(const PString & str) = 0;
    virtual PString ToString() const = 0;

    const OpalMediaCryptoSuite & GetCryptoSuite() const { return m_cryptoSuite; }

    void SetTag(const PString & tag) { m_tag = tag; }
    const PString & GetTag() const { return m_tag; }

  protected:
    const OpalMediaCryptoSuite & m_cryptoSuite;
    PString m_tag;
};

typedef PList<OpalMediaCryptoKeyInfo> OpalMediaCryptoKeyList;


/** Class for desribing the crytpographic mechanism used by an OpalMediaSession.
    These are singletons that descibe the cypto suite in use
  */
class OpalMediaCryptoSuite : public PObject
{
    PCLASSINFO(OpalMediaCryptoSuite, PObject);
  protected:
    OpalMediaCryptoSuite() { }

  public:
    static const PCaselessString & ClearText();

    virtual const PCaselessString & GetFactoryName() const = 0;
    virtual bool Supports(const PCaselessString & proto) const = 0;
    virtual bool ChangeSessionType(PCaselessString & mediaSession) const = 0;

    virtual const char * GetDescription() const = 0;

    virtual OpalMediaCryptoKeyInfo * CreateKeyInfo() const = 0;
};

typedef PFactory<OpalMediaCryptoSuite, PCaselessString> OpalMediaCryptoSuiteFactory;


/** Class for carrying media session information
  */
class OpalMediaSession : public PObject
{
    PCLASSINFO(OpalMediaSession, PObject);
  public:
    /// Initialisation information for constructing a session
    struct Init {
      Init(
        OpalConnection & connection,    ///< Connection that owns the sesion
        unsigned sessionId,             ///< Unique (in connection) session ID for session
        const OpalMediaType & mediaType ///< Media type for session
      ) : m_connection(connection)
        , m_sessionId(sessionId)
        , m_mediaType(mediaType)
      { }


      OpalConnection & m_connection;
      unsigned         m_sessionId;
      OpalMediaType    m_mediaType;
    };

  protected:
    OpalMediaSession(const Init & init);

  public:
    virtual const PCaselessString & GetSessionType() const = 0;
    virtual bool Open(const PString & localInterface);
    virtual bool IsOpen() const;
    virtual bool Close();
    virtual OpalTransportAddress GetLocalMediaAddress() const;
    virtual OpalTransportAddress GetRemoteMediaAddress() const;
    virtual bool SetRemoteMediaAddress(const OpalTransportAddress &);
    virtual OpalTransportAddress GetRemoteControlAddress() const;
    virtual bool SetRemoteControlAddress(const OpalTransportAddress &);

    typedef PList<PChannel> Transport;
    virtual void AttachTransport(Transport & transport);
    virtual Transport DetachTransport();

    bool IsExternalTransport() const { return m_isExternalTransport; }
    virtual void SetExternalTransport(const OpalTransportAddressArray & transports);

#if OPAL_SIP
    virtual SDPMediaDescription * CreateSDPMediaDescription();
#endif

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      bool isSource
    ) = 0;

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool receiver) const;
#endif

    void Use() { m_referenceCount++; }
    bool Release() { return --m_referenceCount > 0; }

    void OfferCryptoSuite(const PString & cryptoSuite);
    OpalMediaCryptoKeyList & GetOfferedCryptoKeys() { return m_offeredCryptokeys; }

    virtual bool ApplyCryptoKey(
      OpalMediaCryptoKeyList & keys,
      bool rx
    );


    unsigned GetSessionID() const { return m_sessionId; }
    const OpalMediaType & GetMediaType() const { return m_mediaType; }

  protected:
    OpalConnection & m_connection;
    unsigned         m_sessionId;  // unique session ID
    OpalMediaType    m_mediaType;  // media type for session
    bool             m_isExternalTransport;
    PAtomicInteger   m_referenceCount;

    OpalMediaCryptoKeyList m_offeredCryptokeys;

  private:
    OpalMediaSession(const OpalMediaSession & other) : PObject(other), m_connection(other.m_connection) { }
    void operator=(const OpalMediaSession &) { }
};


typedef PParamFactory<OpalMediaSession, const OpalMediaSession::Init &, PCaselessString> OpalMediaSessionFactory;

#ifdef OPAL_SRTP
PFACTORY_LOAD(OpalSRTPSession);
#endif


#endif // OPAL_OPAL_MEDIASESSION_H
