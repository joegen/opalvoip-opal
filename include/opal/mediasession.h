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


#if OPAL_STATISTICS

/**This class carries statistics on the media stream.
  */
class OpalMediaStatistics : public PObject
{
    PCLASSINFO(OpalMediaStatistics, PObject);
  public:
    OpalMediaStatistics();

    // General info (typicallly from RTP)
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

    // Video
    unsigned m_totalFrames;
    unsigned m_keyFrames;

    // Fax
#if OPAL_FAX
    enum {
      FaxNotStarted = -2,
      FaxInProgress = -1,
      FaxSuccessful = 0,
      FaxErrorBase  = 1
    };
    struct Fax {
      Fax();

      int  m_result;      // -2=not started, -1=progress, 0=success, >0=ended with error
      int  m_bitRate;     // e.g. 14400, 9600
      int  m_compression; // 0=N/A, 1=T.4 1d, 2=T.4 2d, 3=T.6
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

      PString m_errorText;
    } m_fax;
#endif
};

#endif


/** Class for carrying media session information
  */
class OpalMediaSession : public PObject
{
    PCLASSINFO(OpalMediaSession, PObject);
  public:
    OpalMediaSession(
      OpalConnection & connection,    ///< Connection that owns the sesion
      unsigned sessionId,             ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType ///< Media type for session
    );

    virtual bool Open(const OpalTransportAddress & localAddress);
    virtual bool Close(bool reading);
    virtual OpalTransportAddress GetLocalMediaAddress() const = 0;
    virtual OpalTransportAddress GetRemoteMediaAddress() const = 0;
    virtual void SetRemoteMediaAddress(const OpalTransportAddress &);

    typedef PList<PChannel> Transport;
    virtual void AttachTransport(Transport & transport);
    virtual Transport DetachTransport();

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      bool isSource
    );

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool receiver) const;
#endif

    void Use() { m_referenceCount++; }
    bool Release() { return --m_referenceCount > 0; }

    unsigned GetSessionID() const { return m_sessionId; }
    const OpalMediaType & GetMediaType() const { return m_mediaType; }

  protected:
    OpalConnection & m_connection;
    unsigned         m_sessionId;  // unique session ID
    OpalMediaType    m_mediaType;  // media type for session
    unsigned         m_referenceCount;

  private:
    OpalMediaSession(const OpalMediaSession & other) : PObject(other), m_connection(other.m_connection) { }
    void operator=(const OpalMediaSession &) { }
};


class OpalSecurityMode : public PObject
{
  PCLASSINFO(OpalSecurityMode, PObject);
  public:
    virtual OpalMediaSession * CreateSession(
      OpalConnection & connection,    ///< Connection that owns the sesion
      unsigned sessionId,             ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType ///< Media type for session
    ) = 0;
    virtual bool Open() = 0;
};


#ifdef OPAL_ZRTP

class OpalZRTPStreamInfo {
  public:
    virtual bool Open() = 0;
    virtual OpalMediaSession * CreateSession(
      OpalConnection & connection,    ///< Connection that owns the sesion
      unsigned sessionId,             ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType ///< Media type for session
    ) = 0;
};

class OpalZRTPConnectionInfo {
  public:
    virtual bool Open() = 0;
    virtual OpalMediaSession * CreateSession(
      OpalConnection & connection,    ///< Connection that owns the sesion
      unsigned sessionId,             ///< Unique (in connection) session ID for session
      const OpalMediaType & mediaType ///< Media type for session
    ) = 0;

    PMutex mutex;
};

#endif // OPAL_ZRTP


#endif // OPAL_OPAL_MEDIASESSION_H
