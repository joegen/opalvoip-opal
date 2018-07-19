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
 */

#ifndef OPAL_OPAL_MEDIASESSION_H
#define OPAL_OPAL_MEDIASESSION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <opal/transports.h>
#include <opal/mediatype.h>
#include <ptlib/notifier_ext.h>


class OpalConnection;
class OpalMediaStream;
class OpalMediaFormat;
class OpalMediaFormatList;
class OpalMediaCryptoSuite;
class RTP_TransportWideCongestionControl;
class H235SecurityCapability;
class H323Capability;
class PSTUNClient;


/**String option key to an integer indicating the time in seconds to
   wait for received media. Default 300.
  */
#define OPAL_OPT_MEDIA_RX_TIMEOUT "Media-Rx-Timeout"

/**String option key to an integer indicating the time in seconds to
   count transmit (ICMP) errors. Default 10.
  */
#define OPAL_OPT_MEDIA_TX_TIMEOUT "Media-Tx-Timeout"


#if OPAL_STATISTICS

struct OpalCodecStatistics
{
  OpalCodecStatistics();

  OpalMediaType m_mediaType;
  PString       m_mediaFormat;
  uint32_t      m_SSRC;
  int           m_payloadType;

  PThreadIdentifier m_threadIdentifier;
};

#if OPAL_ICE
struct OpalCandidateStatistics : PNatCandidate
{
  OpalCandidateStatistics(const PNatCandidate & cand);

  virtual void PrintOn(ostream & strm) const;

  bool m_selected;
  unsigned m_nominations;
  PTime    m_lastNomination;

  struct STUN {
    STUN();
    void Count();

    PTime    m_first;
    PTime    m_last;
    unsigned m_count;
  } m_rxRequests, m_txRequests;
};
#endif // OPAL_ICE

struct OpalNetworkStatistics
{
  OpalNetworkStatistics();

  PString              m_transportName;
  OpalTransportAddress m_localAddress;
  OpalTransportAddress m_remoteAddress;

#if OPAL_ICE
  std::vector<OpalCandidateStatistics> m_candidates;
#endif

  PTime    m_startTime;
  uint64_t m_totalBytes;
  unsigned m_totalPackets;
  unsigned m_controlPacketsIn;  // RTCP received for this channel
  unsigned m_controlPacketsOut; // RTCP sent for this channel
  int      m_NACKs;             // (-1 is N/A)
  uint32_t m_rtxSSRC;           // Zero indicates no separate retransmit source
  int      m_rtxPackets;        // (-1 is N/A)
  int      m_rtxDuplicates;     // (-1 is N/A)
  int      m_FEC;               // (-1 is N/A, for tx is number of FEC frame sent, for rx is number of frames recovered via FEC)
  int      m_unrecovered;       // (-1 is N/A) Packets that failed to arrive and could not be recovered via NACK/FEC
  int      m_packetsLost;       // (-1 is N/A) Packets that failed to arrive (as per RTCP Receiver Report specification)
  int      m_maxConsecutiveLost;// (-1 is N/A)
  int      m_packetsOutOfOrder; // (-1 is N/A)
  int      m_lateOutOfOrder;    // (-1 is N/A)
  int      m_packetsTooLate;    // (-1 is N/A)
  int      m_packetOverruns;    // (-1 is N/A)
  int      m_minimumPacketTime; // Milliseconds (-1 is N/A)
  int      m_averagePacketTime; // Milliseconds (-1 is N/A)
  int      m_maximumPacketTime; // Milliseconds (-1 is N/A)
  int      m_averageJitter;     // Milliseconds (-1 is N/A)
  int      m_maximumJitter;     // Milliseconds (-1 is N/A)
  int      m_jitterBufferDelay; // Milliseconds (-1 is N/A)
  int      m_roundTripTime;     // Milliseconds (-1 is N/A)
  int      m_lastPacketRTP;     // (-1 is N/A)
  PTime    m_lastPacketAbsTime;
  PTime    m_lastPacketNetTime;
  PTime    m_lastReportTime;
  unsigned m_targetBitRate;    // As configured, not actual, which is calculated from m_totalBytes
  float    m_targetFrameRate;  // As configured, not actual, which is calculated from m_totalFrames
};

struct OpalVideoStatistics
{
#if OPAL_VIDEO
  OpalVideoStatistics();

  void IncrementFrames(bool key);
  void IncrementUpdateCount(bool full);

  unsigned      m_totalFrames;
  unsigned      m_keyFrames;
  unsigned      m_droppedFrames;
  PTime         m_lastKeyFrameTime;
  unsigned      m_fullUpdateRequests;
  unsigned      m_pictureLossRequests;
  PTime         m_lastUpdateRequestTime;
  PTimeInterval m_updateResponseTime;
  unsigned      m_frameWidth;
  unsigned      m_frameHeight;
  unsigned      m_tsto;             // Temporal/Spatial Trade Off, as configured
  int           m_videoQuality;    // -1 is none, 0 is very good > 0 is progressively worse
#endif
};

struct OpalFaxStatistics
{
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

    OpalFaxStatistics();

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
#endif // OPAL_FAX
};


/**This class carries statistics on the media stream.
  */
class OpalMediaStatistics : public PObject
                          , public OpalCodecStatistics
                          , public OpalNetworkStatistics
                          , public OpalVideoStatistics
                          , public OpalFaxStatistics
{
    PCLASSINFO(OpalMediaStatistics, PObject);
  public:
    OpalMediaStatistics();
    OpalMediaStatistics(const OpalMediaStatistics & other);
    OpalMediaStatistics & operator=(const OpalMediaStatistics & other);

    virtual void PrintOn(ostream & strm) const;

    // To following fields are not copied by
    struct UpdateInfo
    {
      UpdateInfo();

      PTime    m_lastUpdateTime;
      PTime    m_previousUpdateTime;
      uint64_t m_previousBytes;
      unsigned m_previousPackets;
      unsigned m_previousLost;
#if OPAL_VIDEO
      unsigned m_previousFrames;
#endif
      PTimeInterval m_usedCPU;
      PTimeInterval m_previousCPU;
    } m_updateInfo;

    void PreUpdate();
    OpalMediaStatistics & Update(const OpalMediaStream & stream);

    bool IsValid() const;

    unsigned GetRateInt(int64_t current, int64_t previous) const;
    unsigned GetBitRate() const { return GetRateInt(m_totalBytes*8, m_updateInfo.m_previousBytes*8); }
    unsigned GetPacketRate() const { return GetRateInt(m_totalPackets, m_updateInfo.m_previousPackets); }
    unsigned GetLossRate() const { return m_packetsLost <= 0 ? 0 : GetRateInt(m_packetsLost, m_updateInfo.m_previousLost); }

    PString GetRateStr(int64_t total, const char * units = "", unsigned significantFigures = 0) const;
    PString GetRateStr(int64_t current, int64_t previous, const char * units = "", unsigned significantFigures = 0) const;
    PString GetAverageBitRate(const char * units = "", unsigned significantFigures = 0) const { return GetRateStr(m_totalBytes*8, units, significantFigures); }
    PString GetCurrentBitRate(const char * units = "", unsigned significantFigures = 0) const { return GetRateStr(m_totalBytes*8, m_updateInfo.m_previousBytes*8, units, significantFigures); }
    PString GetAveragePacketRate(const char * units = "", unsigned significantFigures = 0) const { return GetRateStr(m_totalPackets, units, significantFigures); }
    PString GetCurrentPacketRate(const char * units = "", unsigned significantFigures = 0) const { return GetRateStr(m_totalPackets, m_updateInfo.m_previousPackets, units, significantFigures); }
    PString GetPacketLossRate(const char * units = "", unsigned significantFigures = 0) const { return GetRateStr(m_packetsLost, m_updateInfo.m_previousLost, units, significantFigures); }

    PString GetCPU() const; // As percentage or one core

#if OPAL_VIDEO
    // Video
    unsigned GetFrameRate() const { return GetRateInt(m_totalFrames, m_updateInfo.m_previousFrames); }
    PString GetAverageFrameRate(const char * units = "", unsigned significantFigures = 0) const;
    PString GetCurrentFrameRate(const char * units = "", unsigned significantFigures = 0) const;
#endif

    // Fax
#if OPAL_FAX
    OpalFaxStatistics & m_fax; // For backward compatibility
#endif // OPAL_FAX

    P_DEPRECATED PString GetRate(int64_t current, int64_t previous, const char * units = "", unsigned significantFigures = 0) const
    { return GetRateStr(current, previous, units, significantFigures);  }
};

#endif


/** Class for contianing the cryptographic keys for use by OpalMediaCryptoSuite.
  */
class OpalMediaCryptoKeyInfo : public PObject
{
    PCLASSINFO(OpalMediaCryptoKeyInfo, PObject);
  protected:
    OpalMediaCryptoKeyInfo(const OpalMediaCryptoSuite & cryptoSuite) : m_cryptoSuite(cryptoSuite) { }

  public:
    virtual ~OpalMediaCryptoKeyInfo() { }

    virtual bool IsValid() const = 0;
    virtual void Randomise() = 0;
    virtual bool FromString(const PString & str) = 0;
    virtual PString ToString() const = 0;

    virtual bool SetCipherKey(const PBYTEArray & key) = 0;
    virtual bool SetAuthSalt(const PBYTEArray & key) = 0;

    virtual PBYTEArray GetCipherKey() const = 0;
    virtual PBYTEArray GetAuthSalt() const = 0;

    const OpalMediaCryptoSuite & GetCryptoSuite() const { return m_cryptoSuite; }

    void SetTag(const PString & tag) { m_tag = tag; }
    const PString & GetTag() const { return m_tag; }

  protected:
    const OpalMediaCryptoSuite & m_cryptoSuite;
    PString m_tag;
};

struct OpalMediaCryptoKeyList : PList<OpalMediaCryptoKeyInfo>
{
  void Select(iterator & it);
};


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

    virtual void PrintOn(ostream & strm) const;

    virtual const PCaselessString & GetFactoryName() const = 0;
    virtual bool Supports(const PCaselessString & proto) const = 0;
    P_DECLARE_BITWISE_ENUM(KeyExchangeModes, 3, (e_NoMode, e_AllowClear, e_SecureSignalling, e_InBandKeyEchange));
    virtual bool ChangeSessionType(PCaselessString & mediaSession, KeyExchangeModes modes) const = 0;

    virtual const char * GetDescription() const = 0;
#if OPAL_H235_6 || OPAL_H235_8
    virtual H235SecurityCapability * CreateCapability(const H323Capability & mediaCapability) const;
    virtual const char * GetOID() const = 0;
    static OpalMediaCryptoSuite * FindByOID(const PString & oid);
#endif

    PINDEX GetCipherKeyBytes() const { return (GetCipherKeyBits()+7)/8; }
    PINDEX GetAuthSaltBytes() const { return (GetAuthSaltBits()+7)/8; }
    virtual PINDEX GetCipherKeyBits() const = 0;
    virtual PINDEX GetAuthSaltBits() const = 0;

    virtual OpalMediaCryptoKeyInfo * CreateKeyInfo() const = 0;

    struct List : PList<OpalMediaCryptoSuite>
    {
      List() { DisallowDeleteObjects(); }
    };
    static List FindAll(
      const PStringArray & cryptoSuiteNames,
      const char * prefix = NULL
    );

  private:
    P_REMOVE_VIRTUAL(H235SecurityCapability *,CreateCapability(const OpalMediaFormat &, unsigned) const,0);
};

typedef PFactory<OpalMediaCryptoSuite, PCaselessString> OpalMediaCryptoSuiteFactory;


struct OpalMediaTransportChannelTypes
{
  enum SubChannels
  {
    e_AllSubChannels = -1,
    e_Media,
    e_Data = e_Media, // for backward compatibility
    e_Control,
    eSubChannelA,
    eSubChannelB,
    eSubChannelC,
    eSubChannelD,
    eMaxSubChannels // Should be enough!
  };
};
#if PTRACING
ostream & operator<<(ostream & strm, OpalMediaTransportChannelTypes::SubChannels channel);
#endif


/** Class for low level transport of media
  */
class OpalMediaTransport : public PSafeObject, public OpalMediaTransportChannelTypes
{
    PCLASSINFO(OpalMediaTransport, PSafeObject);
  private:
    PDECLARE_INSTRUMENTED_READ_WRITE_MUTEX(m_instrumentedMutex, OpalMediaTransport, 500, 100);
  public:
    OpalMediaTransport(const PString & name);
    ~OpalMediaTransport();

    virtual void PrintOn(ostream & strm) const;

    /// Return name of the transport.
    const PString & GetName() const { return m_name; }

    /// Return the type of the transport
    virtual PString GetType();

    /**Open the media transport.
      */
    virtual bool Open(
      OpalMediaSession & session,
      PINDEX count,
      const PString & localInterface,
      const OpalTransportAddress & remoteAddress
    ) = 0;

    /**Indicate transport is open.
      */
    virtual bool IsOpen() const;

    /**Start threads to read from transport subchannels.
      */
    virtual void Start();

    /**Indicate session has completed any initial negotiations.
      */
    virtual bool IsEstablished() const;

    /**Get the local transport address used by this media session.
       The \p subchannel can get an optional secondary channel address
       when false.
      */
    virtual OpalTransportAddress GetLocalAddress(SubChannels subchannel = e_Media) const;

    /**Get the remote transport address used by this media session.
       The \p subchannel can get an optional secondary channel address
       when false.
      */
    virtual OpalTransportAddress GetRemoteAddress(SubChannels subchannel = e_Media) const;

    /**Set the remote transport address used by this media session.
       The \p subchannel can get an optional secondary channel address
       when false.
      */
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, SubChannels subchannel = e_Media);

    /**Set the candidates for use in this media transport.
      */
    virtual void SetCandidates(
      const PString & user,
      const PString & pass,
      const PNatCandidateList & candidates
    );

    /**Get the candidates for use in this media transport.
      */
    virtual bool GetCandidates(
      PString & user,
      PString & pass,
      PNatCandidateList & candidates,
      bool offering
    );

    /**Write to media transport.
      */
    virtual bool Write(
      const void * data,
      PINDEX length,
      SubChannels subchannel = e_Media,
      const PIPSocketAddressAndPort * remote = NULL,
      int * mtu = NULL
    ) = 0;

#if OPAL_SRTP
    /**Get encryption keys.
      */
    virtual bool GetKeyInfo(
      OpalMediaCryptoKeyInfo * keyInfo[2]
    );
#endif

    /**Read data notification.
       If PBYEArray is empty, then the transport has had an error and has been closed.
      */
    typedef PNotifierTemplate<PBYTEArray> ReadNotifier;
    #define PDECLARE_MediaReadNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalMediaTransport, cls, fn, PBYTEArray)

    /** Set the notifier for read data.
      */
    void AddReadNotifier(
      const ReadNotifier & notifier,
      SubChannels subchannel = e_Media
    );

    /// Remove the read notifier
    void RemoveReadNotifier(
      const ReadNotifier & notifier,
      SubChannels subchannel = e_Media
    );
    void RemoveReadNotifier(
      PObject * target,
      SubChannels subchannel = e_Media
    );

    /**Get channel object for subchannel index.
      */
    PChannel * GetChannel(SubChannels subchannel = e_Media) const;

    void SetDiscoverMTU(int mode) { m_mtuDiscoverMode = mode; }
    void SetMediaTimeout(const PTimeInterval & t);
    void SetRemoteBehindNAT();

    /// Congestion control handling
    struct CongestionControl
    {
      virtual ~CongestionControl() { }
      virtual unsigned HandleTransmitPacket(unsigned sessionID, uint32_t ssrc) = 0;
      virtual void HandleReceivePacket(unsigned sn, const PTime & received) = 0;
      virtual PTimeInterval GetProcessInterval() const = 0;
      virtual bool ProcessReceivedPackets() = 0;
      virtual void ProcessTWCC(RTP_TransportWideCongestionControl & twcc) = 0;
    };

    CongestionControl * SetCongestionControl(CongestionControl * cc);
    CongestionControl * GetCongestionControl() const { return m_congestionControl; }

#if OPAL_STATISTICS
    /**Get statistics for this media session.
      */
    virtual void GetStatistics(OpalMediaStatistics & statistics) const;
#endif

  protected:
    virtual void InternalClose();
    virtual void InternalStop();
    virtual void InternalRxData(SubChannels subchannel, const PBYTEArray & data);

    PString       m_name;
    bool          m_remoteBehindNAT;
    PINDEX        m_packetSize;
    int           m_mtuDiscoverMode;
    PTimeInterval m_mediaTimeout;
    PSimpleTimer  m_mediaTimer;
    PTimeInterval m_maxNoTransmitTime;
    atomic<bool>  m_opened;
    atomic<bool>  m_established;
    atomic<bool>  m_started;

    atomic<CongestionControl *> m_congestionControl;
    PTimer m_ccTimer;
    PDECLARE_NOTIFIER(PTimer, OpalMediaTransport, ProcessCongestionControl);

    enum RemoteAddressSources {
      e_RemoteAddressUnknown,
      e_RemoteAddressFromSignalling,
      e_RemoteAddressFromFirstPacket,
      e_RemoteAddressFromProvisionalPair,
      e_RemoteAddressFromICE
    };
#if PTRACING
    friend ostream & operator<<(ostream & strm, RemoteAddressSources source);
#endif

    struct ChannelInfo
    {
      ChannelInfo(
        OpalMediaTransport & owner,
        SubChannels subchannel,
        PChannel * channel
      );

      void ThreadMain();
      bool HandleUnavailableError();

      typedef PNotifierListTemplate<PBYTEArray> NotifierList;
      NotifierList m_notifiers;

      OpalMediaTransport & m_owner;
      SubChannels    const m_subchannel;
      PChannel     * const m_channel;
      PThread            * m_thread;
      unsigned             m_consecutiveUnavailableErrors;
      PSimpleTimer         m_timeForUnavailableErrors;
      OpalTransportAddress m_localAddress;
      OpalTransportAddress m_remoteAddress;
      RemoteAddressSources m_remoteAddressSource;

      PTRACE_THROTTLE(m_throttleReadPacket,4,60000);
      PTRACE_PARAM(bool m_logFirstRead);

#if defined(__GNUC__) && __cplusplus < 201103
      void operator=(const ChannelInfo &) { }
#endif
    };
    friend struct ChannelInfo;
    typedef vector<ChannelInfo> ChannelArray;
    ChannelArray m_subchannels;
    void AddChannel(PChannel * channel);
    virtual PChannel * AddWrapperChannels(SubChannels subchannel, PChannel * channel);
};

typedef PSafePtr<OpalMediaTransport, PSafePtrMultiThreaded> OpalMediaTransportPtr;


class OpalTCPMediaTransport : public OpalMediaTransport
{
  public:
    OpalTCPMediaTransport(const PString & name);
    ~OpalTCPMediaTransport() { InternalStop(); }

    virtual bool Open(OpalMediaSession & session, PINDEX count, const PString & localInterface, const OpalTransportAddress & remoteAddress);
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, PINDEX = e_Media);
    virtual bool Write(const void * data, PINDEX length, SubChannels = e_Media, const PIPSocketAddressAndPort * = NULL, int * = NULL);
};


/** Class for low level transport of media that uses UDP
  */
class OpalUDPMediaTransport : public OpalMediaTransport
{
    PCLASSINFO(OpalUDPMediaTransport, OpalMediaTransport);
  public:
    OpalUDPMediaTransport(const PString & name);
    ~OpalUDPMediaTransport() { InternalStop(); }

    virtual bool Open(OpalMediaSession & session, PINDEX count, const PString & localInterface, const OpalTransportAddress & remoteAddress);
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, SubChannels subchannel = e_Media);
    virtual bool Write(const void * data, PINDEX length, SubChannels = e_Media, const PIPSocketAddressAndPort * = NULL, int * = NULL);

    PUDPSocket * GetSubChannelAsSocket(SubChannels subchannel = e_Media) const;

  protected:
    virtual void InternalRxData(SubChannels subchannel, const PBYTEArray & data);
    virtual bool InternalSetRemoteAddress(const PIPSocket::AddressAndPort & ap, SubChannels subchannel, RemoteAddressSources source);
    virtual bool InternalOpenPinHole(PUDPSocket & socket);

    bool m_localHasRestrictedNAT;
    vector<PUDPSocket *> m_socketCache;
};


///////////////////////////////////////////////////////////////////////////////

/** Class for carrying media session information
  */
class OpalMediaSession : public PSafeObject, public OpalMediaTransportChannelTypes
{
    PCLASSINFO(OpalMediaSession, PSafeObject);
  private:
    PDECLARE_INSTRUMENTED_READ_WRITE_MUTEX(m_instrumentedMutex, OpalMediaSession, 500, 100);
  public:
    /// Initialisation information for constructing a session
    struct Init {
      Init(
        OpalConnection & connection,     ///< Connection that owns the sesion
        unsigned sessionId,              ///< Unique (in connection) session ID for session
        const OpalMediaType & mediaType, ///< Media type for session
        bool remoteBehindNAT
      ) : m_connection(connection)
        , m_sessionId(sessionId)
        , m_mediaType(mediaType)
        , m_remoteBehindNAT(remoteBehindNAT)
      { }


      OpalConnection & m_connection;
      unsigned         m_sessionId;
      OpalMediaType    m_mediaType;
      bool             m_remoteBehindNAT;
    };

  protected:
    OpalMediaSession(const Init & init);

  public:
    ~OpalMediaSession();

    virtual void PrintOn(ostream & strm) const;

    /** Get the session type string (for factory).
      */
    virtual const PCaselessString & GetSessionType() const = 0;

    /**Open the media session.
      */
    virtual bool Open(
      const PString & localInterface,
      const OpalTransportAddress & remoteAddress
    ) = 0;

    /**Indicate if media session is open.
      */
    virtual bool IsOpen() const;

    /**Start reading thread.
      */
    virtual void Start();

    /**Indicate session has completed any initial negotiations.
      */
    virtual bool IsEstablished() const;

    /**Close the media session.
      */
    virtual bool Close();

    /**Get the local transport address used by this media session.
       The \p isMediaAddress can get an optional secondary channel address
       when false.
      */
    virtual OpalTransportAddress GetLocalAddress(bool isMediaAddress = true) const;

    /**Get the remote transport address used by this media session.
       The \p isMediaAddress can get an optional secondary channel address
       when false.
      */
    virtual OpalTransportAddress GetRemoteAddress(bool isMediaAddress = true) const;

    /**Set the remote transport address used by this media session.
       The \p isMediaAddress can get an optional secondary channel address
       when false.
      */
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress = true);

    /**Attach an existing set of transport channels to media session.
      */
    virtual void AttachTransport(const OpalMediaTransportPtr & transport);

    /**Detach the transport channels from the media session.
       Note that while the channels are not closed, the media session will be.
       Also note that the channel objects are now owned by the Transport PList
       so care must be take when removing them in such a way they are not
       deleted unexpectedly.
      */
    virtual OpalMediaTransportPtr DetachTransport();

    /**Get transport channels for the media session.
       This does not detach the tranpsort from the session.
      */
    OpalMediaTransportPtr GetTransport() const { return m_transport; }

    /**Update media stream with media options contained in the media format.
      */
    virtual bool UpdateMediaFormat(
      const OpalMediaFormat & mediaFormat
    );

    static const PString & GetBundleGroupId();

    /**Set the "group" id for the RTP session.
       This is typically a mechanism for connecting audio and video together via BUNDLE.
    */
    virtual bool AddGroup(
        const PString & groupId,  ///< Identifier of the "group"
        const PString & mediaId,  ///< Identifier of the session within the "group"
        bool overwrite = true     ///< Allow overwrite of the mediaId
    );

    /**Indicate if the RTP session is a member of the "group".
       This is typically a mechanism for connecting audio and video together via BUNDLE.
    */
    bool IsGroupMember(
      const PString & groupId
    ) const;

    /**Get all groups this session belongs to.
      */
    PStringArray GetGroups() const;

    /**Get the "group media" id for the group in this RTP session.
       This is typically a mechanism for connecting audio and video together via BUNDLE.
       If not set, uses the media type.
    */
    PString GetGroupMediaId(
      const PString & groupId
    ) const;

#if OPAL_SDP
    /**Create an appropriate SDP media description object for this media session.
      */
    virtual SDPMediaDescription * CreateSDPMediaDescription();
#endif

    /**Create an appropriate media stread for this media session.
      */
    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat,
      unsigned sessionID, 
      bool isSource
    ) = 0;

#if OPAL_STATISTICS
    /**Get statistics for this media session.
      */
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool receiver) const;
#endif

    /// Indicate remote is behind NAT
    void SetRemoteBehindNAT();
    bool IsRemoteBehindNAT() const { return m_remoteBehindNAT; }

    /**Create internal crypto keys for the suite.
      */
    void OfferCryptoSuite(const PString & cryptoSuite);

    /**Get the crypto keys we are offering to remote.
       Note, OfferCryptoSuite() must be called beforehand.
      */
    virtual OpalMediaCryptoKeyList & GetOfferedCryptoKeys();

    /**Apply crypto keys negotiated with remote.
      */
    virtual bool ApplyCryptoKey(
      OpalMediaCryptoKeyList & keys,
      bool rx
    );

    /**Indicate the media session is secured.
       @ retruns the active crypto key for that media
      */
    virtual OpalMediaCryptoKeyInfo * IsCryptoSecured(bool rx) const;

    /**Get the conenction that owns this media session.
      */
    OpalConnection & GetConnection() const { return m_connection; }

    /**Get the identifier number of the media session.
      */
    unsigned GetSessionID() const { return m_sessionId; }

    /**Get the media type of the media session.
      */
    const OpalMediaType & GetMediaType() const { return m_mediaType; }

    /**Get the string options for the media session.
      */
    const PStringOptions & GetStringOptions() const { return m_stringOptions; }

    /**Set the string options for the media session.
      */
    void SetStringOptions(const PStringOptions & options) { m_stringOptions = options; }

  protected:
    OpalConnection & m_connection;
    unsigned         m_sessionId;  // unique session ID
    OpalMediaType    m_mediaType;  // media type for session
    bool             m_remoteBehindNAT;
    PStringOptions   m_stringOptions;
    PStringToString  m_groups;

    OpalMediaTransportPtr  m_transport;
    OpalMediaCryptoKeyList m_offeredCryptokeys;

  private:
    OpalMediaSession(const OpalMediaSession & other) : PSafeObject(other), m_connection(other.m_connection) { }
    void operator=(const OpalMediaSession &) { }

    P_REMOVE_VIRTUAL(bool, Open(const PString &), false);
    P_REMOVE_VIRTUAL(OpalTransportAddress, GetLocalMediaAddress() const, 0);
    P_REMOVE_VIRTUAL(OpalTransportAddress, GetRemoteMediaAddress() const, 0);
    P_REMOVE_VIRTUAL(bool, SetRemoteMediaAddress(const OpalTransportAddress &), false);
    P_REMOVE_VIRTUAL(OpalTransportAddress, GetRemoteControlAddress() const, 0);
    P_REMOVE_VIRTUAL(bool, SetRemoteControlAddress(const OpalTransportAddress &), false);
    P_REMOVE_VIRTUAL_VOID(SetRemoteUserPass(const PString &, const PString &));
};


/**Dummy session.
   This is a place holder for the local and remote address in use for a
   session, but there is no actual implementation that does anything. It
   is used for cases such as unknown media types in SDP or external "bypassed"
   media sessions where data is sent driectly between two remote endpoints and
   not throught the local machine at all.
  */
class OpalDummySession : public OpalMediaSession
{
    PCLASSINFO(OpalDummySession, OpalMediaSession)
  public:
    OpalDummySession(const Init & init);
#if OPAL_SDP
    OpalDummySession(const Init & init, const PStringArray & sdpTokens);
#endif
    OpalDummySession(const Init & init, const OpalTransportAddressArray & transports);
    static const PCaselessString & SessionType();
    virtual const PCaselessString & GetSessionType() const;
    virtual bool Open(const PString & localInterface, const OpalTransportAddress & remoteAddress);
    virtual bool IsOpen() const;
    virtual OpalTransportAddress GetLocalAddress(bool isMediaAddress = true) const;
    virtual OpalTransportAddress GetRemoteAddress(bool isMediaAddress = true) const;
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress = true);
    virtual void AttachTransport(const OpalMediaTransportPtr &);
    virtual OpalMediaTransportPtr DetachTransport();
#if OPAL_SDP
    virtual SDPMediaDescription * CreateSDPMediaDescription();
#endif
    virtual OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource);

  private:
#if OPAL_SDP
    PStringArray         m_sdpTokens;
#endif
    OpalTransportAddress m_localTransportAddress[2];
    OpalTransportAddress m_remoteTransportAddress[2];
};


typedef PParamFactory<OpalMediaSession, const OpalMediaSession::Init &, PCaselessString> OpalMediaSessionFactory;

#if OPAL_SRTP
  PFACTORY_LOAD(OpalSRTPSession);
#endif

#if OPAL_H235_6
  PFACTORY_LOAD(H2356_Session);
#endif

#endif // OPAL_OPAL_MEDIASESSION_H
