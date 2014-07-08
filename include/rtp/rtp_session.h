/*
 * rtp_session.h
 *
 * RTP protocol session
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2012 Equivalence Pty. Ltd.
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
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_RTP_RTP_SESSION_H
#define OPAL_RTP_RTP_SESSION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal_config.h>

#include <rtp/rtp.h>
#include <rtp/jitter.h>
#include <opal/mediasession.h>
#include <opal/mediafmt.h>
#include <ptlib/sockets.h>
#include <ptlib/safecoll.h>
#include <ptlib/notifier_ext.h>
#include <ptclib/pnat.h>
#include <ptclib/url.h>

#include <list>


class OpalRTPEndPoint;
class PNatMethod;
class PSTUNServer;
class PSTUNClient;
class RTCP_XR_Metrics;
class RTP_ExtendedReportArray;


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class OpalRTPSession : public OpalMediaSession
{
    PCLASSINFO(OpalRTPSession, OpalMediaSession);
  public:
    static const PCaselessString & RTP_AVP();
    static const PCaselessString & RTP_AVPF();

  /**@name Construction */
  //@{
    /**Create a new RTP session.
     */
    OpalRTPSession(const Init & init);

    /**Delete a session.
       This deletes the userData field if autoDeleteUserData is true.
     */
    ~OpalRTPSession();
  //@}

  /**@name Overrides from class OpalMediaSession */
  //@{
    virtual const PCaselessString & GetSessionType() const { return RTP_AVP(); }
    virtual bool Open(const PString & localInterface, const OpalTransportAddress & remoteAddress, bool isMediaAddress);
    virtual bool IsOpen() const;
    virtual bool Close();
    virtual bool Shutdown(bool reading);
    virtual OpalTransportAddress GetLocalAddress(bool isMediaAddress = true) const;
    virtual OpalTransportAddress GetRemoteAddress(bool isMediaAddress = true) const;
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress = true);
#if OPAL_ICE
    virtual void SetICE(const PString & user, const PString & pass, const PNatCandidateList & candidates);
#endif

    virtual void AttachTransport(Transport & transport);
    virtual Transport DetachTransport();

    virtual bool UpdateMediaFormat(const OpalMediaFormat & mediaFormat);

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      bool isSource
    );
  //@}

  /**@name Operations */
  //@{
    enum Direction
    {
      e_Receive,
      e_Transmit
    };

    /** Add a syncronisation source.
        If no call is made to this function, then the first sent/received
        RTP_DataFrame packet will set the SSRC for the session.
        if \p id is zero then a new random SSRC is generated.
        @returns SSRC that was added.
      */
    virtual RTP_SyncSourceId AddSyncSource(
      RTP_SyncSourceId id,
      Direction dir,
      const char * cname = NULL
    );

    /**Get the all source identifiers.
      */
    RTP_SyncSourceArray GetSyncSources(Direction dir) const;

    /**Read a data frame from the RTP channel.
       This function will conditionally read data from the jitter buffer or
       directly if there is no jitter buffer enabled.
      */
    virtual bool ReadData(
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
    );

    /**Write a data frame from the RTP channel.
      */
    virtual bool WriteData(
      RTP_DataFrame & frame,                          ///<  Frame to write to the RTP session
      const PIPSocketAddressAndPort * remote = NULL,  ///< Alternate address to transmit data frame
      bool rewriteHeader = true                       /**< Indicate header fields like sequence
                                                           numbers are to be rewritten according to
                                                           session status */
    );

    /**Send a report to remote.
      */
    void SendReport(bool force);

    /**Write a control frame from the RTP channel.
      */
    virtual bool WriteControl(
      RTP_ControlFrame & frame,                      ///<  Frame to write to the RTP session
      const PIPSocketAddressAndPort * remote = NULL  ///< Alternate address to transmit control frame
    );

   /**Restarts an existing session in the given direction.
      */
    virtual void Restart(
      bool isReading
    );

    /**Get the local host name as used in SDES packes.
      */
    virtual PString GetLocalHostName();

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool receiver, RTP_SyncSourceId ssrc) const;
#endif
  //@}

  /**@name Call back functions */
  //@{
    enum SendReceiveStatus {
      e_ProcessPacket,
      e_IgnorePacket,
      e_AbortTransport
    };
    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame, bool rewriteHeader);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame, PINDEX pduSize);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);
    virtual SendReceiveStatus OnOutOfOrderPacket(RTP_DataFrame & frame);

#if OPAL_RTP_FEC
    virtual SendReceiveStatus OnSendRedundantFrame(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnSendRedundantData(RTP_DataFrame & primary, RTP_DataFrameList & redundancies);
    virtual SendReceiveStatus OnReceiveRedundantFrame(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveRedundantData(RTP_DataFrame & primary, RTP_DataFrame::PayloadTypes payloadType, unsigned timestamp, const BYTE * data, PINDEX size);

    struct FecLevel
    {
      PBYTEArray m_mask; // Points to 16 or 48 bits
      PBYTEArray m_data;
    };
    struct FecData
    {
      unsigned m_timestamp;
      bool     m_pRecovery;
      bool     m_xRecovery;
      unsigned m_ccRecovery;
      bool     m_mRecovery;
      unsigned m_ptRecovery;
      unsigned m_snBase;
      unsigned m_tsRecovery;
      unsigned m_lenRecovery;
      vector<FecLevel> m_level;
    };
    virtual SendReceiveStatus OnSendFEC(RTP_DataFrame & primary, FecData & fec);
    virtual SendReceiveStatus OnReceiveFEC(RTP_DataFrame & primary, const FecData & fec);
#endif // OPAL_RTP_FEC

    typedef RTP_SenderReport           SenderReport;
    typedef RTP_ReceiverReport         ReceiverReport;
    typedef RTP_ReceiverReportArray    ReceiverReportArray;
    typedef RTP_SourceDescription      SourceDescription;
    typedef RTP_SourceDescriptionArray SourceDescriptionArray;

    virtual void OnRxSenderReport(const SenderReport & sender, const ReceiverReportArray & reports);
    virtual void OnRxReceiverReport(RTP_SyncSourceId src, const ReceiverReportArray & reports);
    virtual void OnRxReceiverReports(const ReceiverReportArray & reports);
    virtual void OnRxSourceDescription(const SourceDescriptionArray & descriptions);
    virtual void OnRxGoodbye(const RTP_SyncSourceArray & sources, const PString & reason);
    virtual void OnRxNACK(RTP_SyncSourceId ssrc, const std::set<unsigned> lostPackets);
    virtual void OnRxApplDefined(const RTP_ControlFrame::ApplDefinedInfo & info);

    typedef PNotifierListTemplate<const RTP_ControlFrame::ApplDefinedInfo &> ApplDefinedNotifierList;
    typedef PNotifierTemplate<const RTP_ControlFrame::ApplDefinedInfo &> ApplDefinedNotifier;

    void AddApplDefinedNotifier(const ApplDefinedNotifier & notifier)
    {
      m_applDefinedNotifiers.Add(notifier);
    }

    void RemoveApplDefinedNotifier(const ApplDefinedNotifier & notifier)
    {
      m_applDefinedNotifiers.Remove(notifier);
    }

#if OPAL_RTCP_XR
    virtual void OnRxExtendedReport(
      RTP_SyncSourceId src,
      const RTP_ExtendedReportArray & reports
    );
#endif // OPAL_RTCP_XR
  //@}

  /**@name Member variable access */
  //@{
    /**Set flag for single port receive.
       This must be done before Open() is called to take effect.
       This is only for incoming RTP/RTCP, for transmit, simple make the the
       remote data port and remote control port the same.
      */
    void SetSinglePortRx(bool v = true) { m_singlePortRx = v; }

    /**Get flag for single port receive.
      */
    bool IsSinglePortRx() { return m_singlePortRx; }

    /**Set flag for single port transmit.
      */
    void SetSinglePortTx(bool v = true);

    /**Get flag for single port transmit.
    */
    bool IsSinglePortTx() { return m_singlePortTx; }

    /**Get flag for is audio RTP.
      */
    bool IsAudio() const { return m_isAudio; }

    /**Set flag for RTP session is audio.
     */
    void SetAudio(
      bool aud    /// New audio indication flag
    ) { m_isAudio = aud; }

#if OPAL_RTP_FEC
    /// Get the RFC 2198 redundent data payload type
    RTP_DataFrame::PayloadTypes GetRedundencyPayloadType() const { return m_redundencyPayloadType; }

    /// Set the RFC 2198 redundent data payload type
    void SetRedundencyPayloadType(RTP_DataFrame::PayloadTypes pt) { m_redundencyPayloadType = pt; }

    /// Get the RFC 5109 Uneven Level Protection Forward Error Correction payload type
    RTP_DataFrame::PayloadTypes GetUlpFecPayloadType() const { return m_ulpFecPayloadType; }

    /// Set the RFC 5109 Uneven Level Protection Forward Error Correction payload type
    void SetUlpFecPayloadType(RTP_DataFrame::PayloadTypes pt) { m_ulpFecPayloadType = pt; }

    /// Get the RFC 5109 transmit level (number of packets that can be lost)
    unsigned GetUlpFecSendLevel() const { return m_ulpFecSendLevel; }

    /// Set the RFC 2198 redundent data payload type
    void SetUlpFecSendLevel(unsigned level) { m_ulpFecSendLevel = level; }
#endif // OPAL_RTP_FEC

    /**Get the canonical name for the RTP session.
      */
    PString GetCanonicalName(RTP_SyncSourceId ssrc = 0, Direction dir = e_Transmit) const;

    /**Set the canonical name for the RTP session.
      */
    void SetCanonicalName(const PString & name, RTP_SyncSourceId ssrc = 0, Direction dir = e_Transmit);

    /**Get the group id for the RTP session.
    */
    PString GetGroupId() const;

    /**Set the group id for the RTP session.
    */
    void SetGroupId(const PString & id);

    /**Get the tool name for the RTP session.
      */
    PString GetToolName() const;

    /**Set the tool name for the RTP session.
      */
    void SetToolName(const PString & name);

    /**Get the RTP header extension codes for the session.
      */
    RTPExtensionHeaders GetExtensionHeaders() const;

    /**Set the RTP header extension codes for the session.
      */
    void SetExtensionHeader(const RTPExtensionHeaders & ext);

    /**Get the source output identifier.
      */
    RTP_SyncSourceId GetSyncSourceIn() const { return GetSyncSource(0, e_Receive).m_sourceIdentifier; }

    /**Get the source output identifier.
      */
    RTP_SyncSourceId GetSyncSourceOut() const { return GetSyncSource(0, e_Transmit).m_sourceIdentifier; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    void SetAnySyncSource(
      bool allow    ///<  Flag for allow any SSRC values
    ) { m_allowAnySyncSource = allow; }


    /**Get the maximum time we wait for packets from remote.
      */
    const PTimeInterval & GetMaxNoReceiveTime() { return m_maxNoReceiveTime; }

    /**Set the maximum time we wait for packets from remote.
      */
    void SetMaxNoReceiveTime(
      const PTimeInterval & interval ///<  New time interval for reports.
    )  { m_maxNoReceiveTime = interval; }

    /**Get the maximum time we wait for remote to start accepting out packets.
      */
    const PTimeInterval & GetMaxNoTransmitTime() { return m_maxNoTransmitTime; }

    /**Set the maximum time we wait for remote to start accepting out packets.
      */
    void SetMaxNoTransmitTime(
      const PTimeInterval & interval ///<  New time interval for reports.
    )  { m_maxNoTransmitTime = interval; }

    /**Get the maximum out of order packets before flagging it missing.
      */
    PINDEX GetMaxOutOfOrderPackets() { return m_maxOutOfOrderPackets; }

    /**Set the maximum out of order packets before flagging it missing.
      */
    void SetMaxOutOfOrderPackets(
      PINDEX packets ///<  Number of packets.
    )  { m_maxOutOfOrderPackets = packets; }

    /**Get the maximum time to wait for an out of order packet before flagging it missing.
      */
    const PTimeInterval & GetOutOfOrderWaitTime() { return m_waitOutOfOrderTime; }

    /**Set the maximum time to wait for an out of order packet before flagging it missing.
      */
    void SetOutOfOrderWaitTime(
      const PTimeInterval & interval ///<  New time interval for reports.
    )  { m_waitOutOfOrderTime = interval; }

    /**Get the time interval for sending RTCP reports in the session.
      */
    PTimeInterval GetReportTimeInterval() { return m_reportTimer.GetResetTime(); }

    /**Set the time interval for sending RTCP reports in the session.
      */
    void SetReportTimeInterval(
      const PTimeInterval & interval ///<  New time interval for reports.
    )  { m_reportTimer.RunContinuous(interval); }

    /**Get the interval for transmitter statistics in the session.
      */
    unsigned GetTxStatisticsInterval() { return m_txStatisticsInterval; }

    /**Set the interval for transmitter statistics in the session.
      */
    void SetTxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Get the interval for receiver statistics in the session.
      */
    unsigned GetRxStatisticsInterval() { return m_rxStatisticsInterval; }

    /**Set the interval for receiver statistics in the session.
      */
    void SetRxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Get local data port of session.
      */
    virtual WORD GetLocalDataPort() const { return m_localPort[e_Data]; }

    /**Get local control port of session.
      */
    virtual WORD GetLocalControlPort() const { return m_localPort[e_Control]; }

    /**Get remote data port of session.
      */
    virtual WORD GetRemoteDataPort() const { return m_remotePort[e_Data]; }

    /**Get remote control port of session.
      */
    virtual WORD GetRemoteControlPort() const { return m_remotePort[e_Control]; }

    /**Get data UDP socket of session.
      */
    virtual PUDPSocket & GetDataSocket() { return *m_socket[e_Data]; }

    /**Get control UDP socket of session.
      */
    virtual PUDPSocket & GetControlSocket() { return *m_socket[e_Control]; }

    /**Get total number of packets sent in session.
      */
    unsigned GetPacketsSent(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_packets; }

    /**Get total number of octets sent in session.
      */
    uint64_t GetOctetsSent(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_octets; }

    /**Get total number of packets received in session.
      */
    unsigned GetPacketsReceived(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_packets; }

    /**Get total number of octets received in session.
      */
    uint64_t GetOctetsReceived(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_octets; }

    /**Get total number received packets lost in session.
      */
    unsigned GetPacketsLost(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_packetsLost; }

    /**Get total number transmitted packets lost by remote in session.
       Determined via RTCP.
      */
    unsigned GetPacketsLostByRemote(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_packetsLost; }

    /**Get total number of packets received out of order in session.
      */
    unsigned GetPacketsOutOfOrder(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_packetsOutOfOrder; }

    /**Get average time between sent packets.
       This is averaged over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    unsigned GetAverageSendTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_averagePacketTime; }

    /**Get the number of marker packets received this session.
       This can be used to find out the number of frames received in a video
       RTP stream.
      */
    unsigned GetMarkerRecvCount(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_markerCount; }

    /**Get the number of marker packets sent this session.
       This can be used to find out the number of frames sent in a video
       RTP stream.
      */
    unsigned GetMarkerSendCount(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_markerCount; }

    /**Get maximum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    unsigned GetMaximumSendTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_maximumPacketTime; }

    /**Get minimum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    unsigned GetMinimumSendTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_minimumPacketTime; }

    /**Get average time between received packets.
       This is averaged over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    unsigned GetAverageReceiveTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_averagePacketTime; }

    /**Get maximum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    unsigned GetMaximumReceiveTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_maximumPacketTime; }

    /**Get minimum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    unsigned GetMinimumReceiveTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_minimumPacketTime; }

    /**Get averaged jitter time for received packets.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    unsigned GetAvgJitterTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_jitter; }

    /**Get averaged jitter time for received packets.
       This is the maximum value of jitterLevel for the session.
      */
    unsigned GetMaxJitterTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receive).m_maximumJitter; }

    /**Get jitter time for received packets on remote.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    unsigned GetJitterTimeOnRemote(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_jitter; }

    /**Get round trip time to remote.
       This is calculated according to the RFC 3550 algorithm.
      */
    unsigned GetRoundTripTime() const { return m_roundTripTime; }
  //@}

    /// Send BYE command
    virtual void SendBYE(RTP_SyncSourceId ssrc = 0);

    virtual bool SendNACK(const std::set<unsigned> & lostPackets, RTP_SyncSourceId ssrc = 0);

    /**Send flow control Request/Notification.
       This uses Temporary Maximum Media Stream Bit Rate from RFC 5104.
       If \p notify is false, and TMMBR is not available, and the Google
       specific REMB is available, that is sent instead.
      */
    virtual bool SendFlowControl(
      unsigned maxBitRate,    ///< New temporary maximum bit rate
      unsigned overhead = 0,  ///< Protocol overhead, defaults to IP/UDP/RTP header size
      bool notify = false,    ///< Send request/notification
      RTP_SyncSourceId ssrc = 0
    );

#if OPAL_VIDEO
    /** Tell the rtp session to send out an intra frame request control packet.
        This is called when the media stream receives an OpalVideoUpdatePicture
        media command.
      */
    virtual bool SendIntraFrameRequest(unsigned options, RTP_SyncSourceId ssrc = 0);

    /** Tell the rtp session to send out an temporal spatial trade off request
        control packet. This is called when the media stream receives an
        OpalTemporalSpatialTradeOff media command.
      */
    virtual bool SendTemporalSpatialTradeOff(unsigned tradeOff, RTP_SyncSourceId ssrc = 0);
#endif

    RTP_Timestamp GetLastSentTimestamp(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_lastTimestamp; }
    const PTimeInterval & GetLastSentPacketTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Transmit).m_lastPacketTick; }

    typedef PNotifierTemplate<SendReceiveStatus &> FilterNotifier;
    #define PDECLARE_RTPFilterNotifier(cls, fn) PDECLARE_NOTIFIER2(RTP_DataFrame, cls, fn, OpalRTPSession::SendReceiveStatus &)
    #define PCREATE_RTPFilterNotifier(fn) PCREATE_NOTIFIER2(fn, OpalRTPSession::SendReceiveStatus &)

    void AddFilter(const FilterNotifier & filter);
    void SetJitterBuffer(OpalJitterBufferPtr jitterBuffer, RTP_SyncSourceId ssrc = 0);

    virtual bool WriteRawPDU(
      const BYTE * framePtr,
      PINDEX frameSize,
      bool toDataChannel,
      const PIPSocketAddressAndPort * remote = NULL
    );

  protected:
    ReceiverReportArray BuildReceiverReportArray(const RTP_ControlFrame & frame, PINDEX offset);
    virtual SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame);
    
    virtual bool InternalSetRemoteAddress(const PIPSocket::AddressAndPort & ap, bool isMediaAddress PTRACE_PARAM(, const char * source));
    virtual SendReceiveStatus InternalReadData(RTP_DataFrame & frame);
    virtual SendReceiveStatus InternalReadControl();
    virtual SendReceiveStatus ReadRawPDU(
      BYTE * framePtr,
      PINDEX & frameSize,
      bool fromDataChannel
    );
    virtual bool HandleUnreachable(PTRACE_PARAM(const char * channelName));

    OpalRTPEndPoint   & m_endpoint;
    bool                m_singlePortRx;
    bool                m_singlePortTx;
    bool                m_isAudio;
    unsigned            m_timeUnits;
    PString             m_groupId;
    PString             m_toolName;
    RTPExtensionHeaders m_extensionHeaders;
    bool                m_allowAnySyncSource;
    PTimeInterval       m_maxNoReceiveTime;
    PTimeInterval       m_maxNoTransmitTime;
    PINDEX              m_maxOutOfOrderPackets; // Number of packets before we give up waiting for an out of order packet
    PTimeInterval       m_waitOutOfOrderTime;   // Milliseconds before we give up on an out of order packet
    unsigned            m_txStatisticsInterval;
    unsigned            m_rxStatisticsInterval;
#if OPAL_RTP_FEC
    RTP_DataFrame::PayloadTypes m_redundencyPayloadType;
    RTP_DataFrame::PayloadTypes m_ulpFecPayloadType;
    unsigned                    m_ulpFecSendLevel;
#endif
    OpalMediaFormat::RTCPFeedback m_feedback;

    friend struct SyncSource;
    struct SyncSource
    {
      SyncSource(OpalRTPSession & session, RTP_SyncSourceId id, Direction dir, const char * cname);
      virtual ~SyncSource();

      virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame, bool rewriteHeader);
      virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame, PINDEX pduSiz);
      virtual SendReceiveStatus OnOutOfOrderPacket(RTP_DataFrame & frame);
      virtual SendReceiveStatus GetPendingFrame(RTP_DataFrame & frame);

      virtual void CalculateStatistics(const RTP_DataFrame & frame);
      virtual void GetStatistics(OpalMediaStatistics & statistics) const;

      virtual void OnSendReceiverReport(RTP_ControlFrame::ReceiverReport & report, SyncSource & sender);
      virtual void OnRxSenderReport(const RTP_SenderReport & report);
      virtual void OnRxReceiverReport(const ReceiverReport & report);

      OpalRTPSession  & m_session;
      Direction         m_direction;
      RTP_SyncSourceId  m_sourceIdentifier;
      PString           m_canonicalName;

      // Sequence handling
      RTP_SequenceNumber m_lastSequenceNumber;
      unsigned           m_lastFIRSequenceNumber;
      unsigned           m_lastTSTOSequenceNumber;
      unsigned           m_consecutiveOutOfOrderPackets;
      PSimpleTimer       m_waitOutOfOrderTimer;
      RTP_DataFrameList  m_pendingPackets;

      // Generating real time stamping in RTP packets as indicated by remote
      // Only useful if local and remote system clocks are well synchronised
      unsigned m_syncTimestamp;
      PTime    m_syncRealTime;

      // Statistics gathered
      PTime    m_firstPacketTime;
      unsigned m_packets;
      uint64_t m_octets;
      unsigned m_senderReports;
      unsigned m_packetsLost;
      unsigned m_packetsOutOfOrder;

      unsigned m_averagePacketTime; // Milliseconds
      unsigned m_maximumPacketTime; // Milliseconds
      unsigned m_minimumPacketTime; // Milliseconds
      unsigned m_jitter;            // Milliseconds
      unsigned m_maximumJitter;     // Milliseconds
      unsigned m_markerCount;

      // Working for calculating statistics
      RTP_Timestamp      m_lastTimestamp;
      PTimeInterval      m_lastPacketTick;

      unsigned           m_averageTimeAccum;
      unsigned           m_maximumTimeAccum;
      unsigned           m_minimumTimeAccum;
      unsigned           m_jitterAccum;
      RTP_Timestamp      m_lastJitterTimestamp;

      // Things to remember for filling in fields of sent RR
      unsigned           m_packetsLostSinceLastRR;
      RTP_SequenceNumber m_lastRRSequenceNumber;

      // For e_Receive, arrival time of last SR from remote
      // For e_Transmit, time we sent last RR to remote
      PTime              m_lastReportTime;

      unsigned           m_statisticsCount;

#if OPAL_RTCP_XR
      RTCP_XR_Metrics * m_metrics; // Calculate the VoIP Metrics for RTCP-XR
#endif

      OpalJitterBufferPtr m_jitterBuffer;
    };
    typedef std::map<RTP_SyncSourceId, SyncSource *> SyncSourceMap;
    SyncSourceMap m_SSRC;
    SyncSource    m_dummySyncSource;
    const SyncSource & GetSyncSource(RTP_SyncSourceId ssrc, Direction dir) const;
    virtual SyncSource * CreateSyncSource(RTP_SyncSourceId id, Direction dir, const char * cname);
    virtual bool GetSyncSource(RTP_SyncSourceId ssrc, Direction dir, SyncSource * & info) const;
    virtual bool CheckControlSSRC(RTP_SyncSourceId senderSSRC, RTP_SyncSourceId targetSSRC, SyncSource * & info PTRACE_PARAM(, const char * pduName)) const;

    /// Set up RTCP as per RFC rules
    virtual void InitialiseControlFrame(RTP_ControlFrame & report, SyncSource & ssrc);

    // Some statitsics not SSRC related
    unsigned m_rtcpPacketsSent;
    unsigned m_roundTripTime;

    PMutex m_reportMutex;
    PTimer m_reportTimer;
    PDECLARE_NOTIFIER(PTimer, OpalRTPSession, TimedSendReport);

    PMutex m_dataMutex;
    PMutex m_readMutex;

    list<FilterNotifier> m_filters;

    enum
    {
      e_Control,
      e_Data
    };
    PIPAddress m_localAddress;
    WORD       m_localPort[2];

    PIPAddress m_remoteAddress;
    WORD       m_remotePort[2];

    PUDPSocket * m_socket[2]; // 0=control, 1=data

    bool m_shutdownRead;
    bool m_shutdownWrite;
    bool m_remoteBehindNAT;
    bool m_localHasRestrictedNAT;
    bool m_firstControl;

    unsigned     m_noTransmitErrors;
    PSimpleTimer m_noTransmitTimer;

    ApplDefinedNotifierList m_applDefinedNotifiers;

#if OPAL_ICE
    virtual SendReceiveStatus OnReceiveICE(
      bool fromDataChannel,
      const BYTE * framePtr,
      PINDEX frameSize,
      const PIPSocket::AddressAndPort & ap
    );
    virtual SendReceiveStatus OnSendICE(
      bool toDataChannel
    );

    struct CandidateState {
      PIPSocketAddressAndPort m_remoteAP;
      // Not sure what else might be necessary here. Work in progress!

      CandidateState(const PIPSocketAddressAndPort& addressAndPort)
        : m_remoteAP(addressAndPort)
      {
      }
    };
    typedef std::list<CandidateState> CandidateStates;

    CandidateStates m_candidates[2];
    PSTUNServer   * m_stunServer;
    PSTUNClient   * m_stunClient;
#endif // OPAL_ICE

#if PTRACING
    unsigned m_levelTxRR;
    unsigned m_levelRxSR;
    unsigned m_levelRxRR;
    unsigned m_levelRxSDES;
    unsigned m_levelTxRED;
    unsigned m_levelRxRED;
    unsigned m_levelRxUnknownFEC;
#endif

  private:
    OpalRTPSession(const OpalRTPSession &);
    void operator=(const OpalRTPSession &) { }

    P_REMOVE_VIRTUAL(int,WaitForPDU(PUDPSocket&,PUDPSocket&,const PTimeInterval&),0);
    P_REMOVE_VIRTUAL(SendReceiveStatus,ReadDataOrControlPDU(BYTE *,PINDEX,bool),e_AbortTransport);
    P_REMOVE_VIRTUAL(bool,WriteDataOrControlPDU(const BYTE *,PINDEX,bool),false);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnSendData(RTP_DataFrame &),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnReceiveData(RTP_DataFrame &),e_AbortTransport);

  friend class RTCP_XR_Metrics;
};


#endif // OPAL_RTP_RTP_H

/////////////////////////////////////////////////////////////////////////////
