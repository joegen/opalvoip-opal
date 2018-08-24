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
class RTP_MetricsReport;


/**OpalConnection::StringOption key to a boolean indicating the AbsSendTime
   header extension (http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time)
   can be used. Default false.
  */
#define OPAL_OPT_RTP_ABS_SEND_TIME "RTP-Abs-Send-Time"

/**OpalConnection::StringOption key to a boolean indicating the transport
   wide congestion control header extension and RTCP support
   (http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions)
   can be used. Default false.
  */
#define OPAL_OPT_TRANSPORT_WIDE_CONGESTION_CONTROL "Transport-Wide-Congestion-Control"


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
    virtual bool Open(const PString & localInterface, const OpalTransportAddress & remoteAddress);
    virtual bool Close();
    virtual OpalTransportAddress GetLocalAddress(bool isMediaAddress = true) const;
    virtual OpalTransportAddress GetRemoteAddress(bool isMediaAddress = true) const;
    virtual bool SetRemoteAddress(const OpalTransportAddress & remoteAddress, bool isMediaAddress = true);
    virtual void AttachTransport(const OpalMediaTransportPtr & transport);
    virtual OpalMediaTransportPtr DetachTransport();
    virtual bool AddGroup(const PString & groupId, const PString & mediaId, bool overwrite = true);

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
      e_Receiver,
      e_Sender
    };
#if PTRACING
    friend ostream & operator<<(ostream & strm, Direction dir);
#endif

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

    /** Remove the synchronisation source.
      */
    virtual bool RemoveSyncSource(
      RTP_SyncSourceId id
      PTRACE_PARAM(, const char * reason)
    );

    /**Get the all source identifiers.
      */
    RTP_SyncSourceArray GetSyncSources(Direction dir) const;

    /**Set the "rtx" SSRC to use for the given SSRC.
       @return rtxSSRC or a newly allocated SSRC used for "rtx" packets. Zero on error.
      */
    RTP_SyncSourceId EnableSyncSourceRtx(
      RTP_SyncSourceId primarySSRC,      ///< Primary SSRC of data that can be retransmitted
      RTP_DataFrame::PayloadTypes rtxPT, ///< Payload type of retramitted packet
      RTP_SyncSourceId rtxSSRC           ///< SSRC of re-transmitted data, 0 indicates allocate new one
    );


    enum SendReceiveStatus {
      e_IgnorePacket = -1,
      e_AbortTransport, // Abort is zero so equvalent to false
      e_ProcessPacket
    };

    enum RewriteMode
    {
      e_RewriteHeader,
      e_RewriteSSRC,
      e_RewriteNothing,
      e_RetransmitFirst,
      e_RetransmitAgain
    };

    enum ReceiveType {
      e_RxFromNetwork,
      e_RxOutOfOrder,
      e_RxRetransmit,
      e_RxFromRTX
    };

    /**Write a data frame from the RTP channel.
      */
    virtual SendReceiveStatus WriteData(
      RTP_DataFrame & frame,                          ///<  Frame to write to the RTP session
      RewriteMode rewrite = e_RewriteHeader,          ///< Indicate what headers are to be rewritten
      const PIPSocketAddressAndPort * remote = NULL,  ///< Alternate address to transmit data frame
      const PTime & now = PTime()
    );

    /**Send a report to remote.
      */
    virtual SendReceiveStatus SendReport(
      RTP_SyncSourceId ssrc,    ///< Zero means all senders, not just first.
      bool force,               ///< Send even if no packets sent yet
      const PTime & now = PTime()
    );

    /**Write a control frame from the RTP channel.
      */
    virtual SendReceiveStatus WriteControl(
      RTP_ControlFrame & frame,                      ///<  Frame to write to the RTP session
      const PIPSocketAddressAndPort * remote = NULL  ///< Alternate address to transmit control frame
    );

    /**Get the local host name as used in SDES packes.
      */
    virtual PString GetLocalHostName();

#if OPAL_STATISTICS
    virtual void GetStatistics(OpalMediaStatistics & statistics, Direction dir) const;
#endif
  //@}

  /**@name Call back functions */
  //@{
    struct Data
    {
      Data(const RTP_DataFrame & frame)
        : m_frame(frame)
        , m_status(e_ProcessPacket)
        { }
      RTP_DataFrame     m_frame;
      SendReceiveStatus m_status;
    };
    typedef PNotifierTemplate<Data &> DataNotifier;
    #define PDECLARE_RTPDataNotifier(cls, fn) PDECLARE_NOTIFIER2(OpalRTPSession, cls, fn, OpalRTPSession::Data &)
    #define PCREATE_RTPDataNotifier(fn)       PCREATE_NOTIFIER2(fn, OpalRTPSession::Data &)

    /** Set the notifier for received RTP data.
        Note an SSRC of zero usually means first SSRC, in this case it measn all SSRC's
      */
    void AddDataNotifier(
      unsigned priority,
      const DataNotifier & notifier,
      RTP_SyncSourceId ssrc = 0
    );

    /// Remove the data notifier
    void RemoveDataNotifier(
      const DataNotifier & notifier,
      RTP_SyncSourceId ssrc = 0
    );

    virtual SendReceiveStatus OnSendData(RewriteMode & rewrite, RTP_DataFrame & frame, const PTime & now);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, const PTime & now);
    virtual SendReceiveStatus OnPreReceiveData(RTP_DataFrame & frame, const PTime & now);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame, ReceiveType rxType, const PTime & now);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame, const PTime & now);

    virtual void OnRxSenderReport(const RTP_SenderReport & sender, const PTime & now);
    virtual void OnRxReceiverReports(RTP_SyncSourceId src, const RTP_ControlFrame::ReceiverReport * rr, unsigned count, const PTime & now);
    virtual void OnRxReceiverReports(RTP_SyncSourceId src, const std::vector<RTP_ReceiverReport> & reports);
    virtual void OnRxReceiverReport(RTP_SyncSourceId src, const RTP_ControlFrame::ReceiverReport & rr);
    virtual void OnRxReceiverReport(RTP_SyncSourceId src, const RTP_ReceiverReport & report);
    virtual void OnRxSourceDescription(const RTP_SourceDescriptionArray & descriptions);
    virtual void OnRxGoodbye(const RTP_SyncSourceArray & sources, const PString & reason);
    virtual void OnRxNACK(RTP_SyncSourceId ssrc, const RTP_ControlFrame::LostPacketMask & lostPackets, const PTime & now);
    virtual void OnRxTWCC(const RTP_TransportWideCongestionControl & twcc);
    virtual void OnRxApplDefined(const RTP_ControlFrame::ApplDefinedInfo & info);
    virtual bool OnReceiveExtendedReports(const RTP_ControlFrame & frame, const PTime & now);
    virtual void OnRxReceiverReferenceTimeReport(RTP_SyncSourceId ssrc, const PTime & ntp, const PTime & now);
    virtual void OnRxDelayLastReceiverReport(const RTP_DelayLastReceiverReport & dlrr, const PTime & now);

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
    virtual void OnRxMetricsReport(
      RTP_SyncSourceId src,
      const RTP_MetricsReport & report
    );
#endif // OPAL_RTCP_XR
  //@}

  /**@name Member variable access */
  //@{
    /**Set flag for single port receive.
       This must be done before Open() is called to take effect.
      */
    void SetSinglePortRx(bool v = true);

    /**Get flag for single port receive.
      */
    bool IsSinglePortRx() { return m_singlePortRx; }

    /**Set flag for single port transmit.
      */
    void SetSinglePortTx(bool v = true);

    /**Get flag for single port transmit.
    */
    bool IsSinglePortTx() { return m_singlePortTx; }

    /**Set flag for RFC5506 reduced size RTCP.
      */
    void SetReducedSizeRTCP(bool v = true) { m_reducedSizeRTCP = v; }

    /**Get flag for RFC5506 reduced size RTCP.
    */
    bool UseReducedSizeRTCP() const { return m_reducedSizeRTCP; }

    /**Get flag for is audio RTP.
      */
    bool IsAudio() const { return m_isAudio; }

    /**Set flag for RTP session is audio.
     */
    void SetAudio(
      bool aud    /// New audio indication flag
    ) { m_isAudio = aud; }

#if OPAL_RTP_FEC
    struct FecLevel
    {
      PBYTEArray m_mask; // Points to 16 or 48 bits
      PBYTEArray m_data;
    };
    struct FecData
    {
      RTP_Timestamp    m_timestamp;
      bool             m_pRecovery;
      bool             m_xRecovery;
      unsigned         m_ccRecovery;
      bool             m_mRecovery;
      unsigned         m_ptRecovery;
      unsigned         m_snBase;
      unsigned         m_tsRecovery;
      unsigned         m_lenRecovery;
      vector<FecLevel> m_level;
    };

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

    /**Get the label for the RTP session.
      */
    PString GetLabel() const;

    /**Set the label for the RTP session.
      */
    void SetLabel(const PString & name);

    /**Get the canonical name for the RTP session.
      */
    PString GetCanonicalName(RTP_SyncSourceId ssrc = 0, Direction dir = e_Sender) const;

    /**Set the canonical name for the RTP session.
      */
    void SetCanonicalName(const PString & name, RTP_SyncSourceId ssrc = 0, Direction dir = e_Sender);

    /**Get the "MediaStream" id for the RTP session SSRC.
       See http://tools.ietf.org/html/draft-ietf-mmusic-msid-12
    */
    PString GetMediaStreamId(RTP_SyncSourceId ssrc, Direction dir) const;

    /**Set the "MediaStream" id for the RTP session SSRC.
       See http://tools.ietf.org/html/draft-ietf-mmusic-msid-12
    */
    void SetMediaStreamId(const PString & id, RTP_SyncSourceId ssrc, Direction dir);

    /**Get the "MediaStreamTrack" id for the RTP session SSRC.
       See http://tools.ietf.org/html/draft-ietf-mmusic-msid-12
    */
    PString GetMediaTrackId(RTP_SyncSourceId ssrc, Direction dir) const;

    /**Set the "MediaStreamTrack" id for the RTP session SSRC.
       See http://tools.ietf.org/html/draft-ietf-mmusic-msid-12
    */
    void SetMediaTrackId(const PString & id, RTP_SyncSourceId ssrc, Direction dir);

    /**Get the SSRC for the secondary rtx packets.
       If \p primary is true then \p ssrc is a primary SSRC and the function
       returns the SSRC that is being used for "rtx" packets. When false,
       \p ssrc is expected to be an "rtx" SSRC and the function returns the
       SSRC used for primary data.
       @return zero if \p ssrc does not exist or is not the expected type.
      */
    RTP_SyncSourceId GetRtxSyncSource(
      RTP_SyncSourceId ssrc,    ///< SSRC to get value for
      Direction dir,            ///< Direction of media, if ssrc == 0
      bool isPrimary            ///< The \p ssrc is a primary and we are getting "rtx" SSRC
  ) const;

    /**Get the tool name for the RTP session.
      */
    PString GetToolName() const;

    /**Set the tool name for the RTP session.
      */
    void SetToolName(const PString & name);

    /**Get the RTP header extension codes for the session.
      */
    RTPHeaderExtensions GetHeaderExtensions() const;

    /**Set the RTP header extension codes for the session.
      */
    void SetHeaderExtensions(const RTPHeaderExtensions & ext);

    /**Add the RTP header extension code to the session.
       @return false if unsupported, or ID already present.
      */
    bool AddHeaderExtension(const RTPHeaderExtensionInfo & ext);

    static const PString & GetAbsSendTimeHdrExtURI();
    static const PString & GetTransportWideSeqNumHdrExtURI();

    /**Get the source identifier for remote data to us.
      */
    RTP_SyncSourceId GetSyncSourceIn() const { return GetSyncSource(0, e_Receiver).m_sourceIdentifier; }

    /**Get the source identifier for our data to the remote.
      */
    RTP_SyncSourceId GetSyncSourceOut() const { return GetSyncSource(0, e_Sender).m_sourceIdentifier; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    void SetAnySyncSource(
      bool allow    ///<  Flag for allow any SSRC values
    );


    /**Get the maximum out of order packets before flagging it missing.
      */
    PINDEX GetMaxOutOfOrderPackets() { return m_maxOutOfOrderPackets; }

    /**Set the maximum out of order packets before flagging it missing.
      */
    void SetMaxOutOfOrderPackets(
      PINDEX packets ///<  Number of packets.
    );

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
    ) { m_txStatisticsInterval = packets; }

    /**Get the interval for receiver statistics in the session.
      */
    unsigned GetRxStatisticsInterval() { return m_rxStatisticsInterval; }

    /**Set the interval for receiver statistics in the session.
      */
    void SetRxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    ) { m_rxStatisticsInterval = packets; }

    /**Get local data port of session.
      */
    virtual WORD GetLocalDataPort() const;

    /**Get local control port of session.
      */
    virtual WORD GetLocalControlPort() const;

    /**Get total number of packets sent in session.
      */
    unsigned GetPacketsSent(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_packets; }

    /**Get total number of octets sent in session.
      */
    uint64_t GetOctetsSent(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_octets; }

    /**Get total number of packets received in session.
      */
    unsigned GetPacketsReceived(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_packets; }

    /**Get total number of octets received in session.
      */
    uint64_t GetOctetsReceived(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_octets; }

    /**Get total number received packets lost in session.
      */
    unsigned GetPacketsLost(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_packetsUnrecovered; }

    /**Get total number transmitted packets lost by remote in session.
       Determined via RTCP. -1 indicates no RTCP received yet.
      */
    int GetPacketsLostByRemote(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_packetsMissing; }

    /**Get total number of packets received out of order in session.
      */
    unsigned GetPacketsOutOfOrder(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_packetsOutOfOrder; }

    /**Get average time between sent packets.
       This is averaged over the last txStatisticsInterval packets and is in
       milliseconds. -1 indicated no packets sent.
      */
    int GetAverageSendTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_averagePacketTime; }

    /**Get the number of marker packets received this session.
       This can be used to find out the number of frames received in a video
       RTP stream.
      */
    unsigned GetMarkerRecvCount(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_markerCount; }

    /**Get the number of marker packets sent this session.
       This can be used to find out the number of frames sent in a video
       RTP stream.
      */
    unsigned GetMarkerSendCount(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_markerCount; }

    /**Get maximum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds. -1 indicated no packets sent.
      */
    int GetMaximumSendTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_maximumPacketTime; }

    /**Get minimum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds. -1 indicated no packets sent.
      */
    int GetMinimumSendTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_minimumPacketTime; }

    /**Get average time between received packets.
       This is averaged over the last rxStatisticsInterval packets and is in
       milliseconds. -1 indicated no packets received.
      */
    int GetAverageReceiveTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_averagePacketTime; }

    /**Get maximum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds. -1 indicated no packets received.
      */
    int GetMaximumReceiveTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_maximumPacketTime; }

    /**Get minimum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds. -1 indicated no packets received.
      */
    int GetMinimumReceiveTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_minimumPacketTime; }

    /**Get averaged jitter time for received packets.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds. -1 indicated no packets received.
      */
    int GetAvgJitterTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_currentjitter; }

    /**Get averaged jitter time for received packets.
       This is the maximum value of jitterLevel for the session. -1 indicated no packets received.
      */
    int GetMaxJitterTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Receiver).m_maximumJitter; }

    /**Get jitter time for received packets on remote.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds. -1 indicated no RTCP received.
      */
    int GetJitterTimeOnRemote(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_currentjitter; }

    /**Get round trip time to remote.
       This is calculated according to the RFC 3550 algorithm.
      */
    int GetRoundTripTime() const { return m_roundTripTime; }
  //@}

    /// Send BYE command
    virtual SendReceiveStatus SendBYE(RTP_SyncSourceId ssrc = 0);

    /// Send NACK RTCP command
    virtual SendReceiveStatus SendNACK(const RTP_ControlFrame::LostPacketMask & lostPackets, RTP_SyncSourceId ssrc = 0);

    /// Send Transport Wide Congestion Control RTCP command
    virtual SendReceiveStatus SendTWCC(const RTP_TransportWideCongestionControl & twcc);

    /**Send flow control Request/Notification.
       This uses Temporary Maximum Media Stream Bit Rate from RFC 5104.
       If \p notify is false, and TMMBR is not available, and the Google
       specific REMB is available, that is sent instead.
      */
    virtual SendReceiveStatus SendFlowControl(
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
    virtual SendReceiveStatus SendIntraFrameRequest(
      unsigned options,
      RTP_SyncSourceId ssrc = 0,
      const PTime & now = PTime()
    );

    /** Tell the rtp session to send out an temporal spatial trade off request
        control packet. This is called when the media stream receives an
        OpalTemporalSpatialTradeOff media command.
      */
    virtual SendReceiveStatus SendTemporalSpatialTradeOff(unsigned tradeOff, RTP_SyncSourceId ssrc = 0);
#endif

    RTP_Timestamp GetLastSentTimestamp(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_lastPacketTimestamp; }
    const PTime & GetLastSentNetTime(RTP_SyncSourceId ssrc = 0) const { return GetSyncSource(ssrc, e_Sender).m_lastPacketNetTime; }

    /// Set the jitter buffer to get certain RTCP statustics from.
    void SetJitterBuffer(OpalJitterBuffer * jitterBuffer, RTP_SyncSourceId ssrc = 0);

    bool HasFeedback(OpalMediaFormat::RTCPFeedback feature) const { return m_feedback&feature; }

  protected:
    virtual OpalMediaTransport * CreateMediaTransport(const PString & name);
    void InternalAttachTransport(const OpalMediaTransportPtr & transport PTRACE_PARAM(, const char * from));

    bool SetQoS(const PIPSocket::QoS & qos);

    PDECLARE_MediaReadNotifier(OpalRTPSession, OnRxDataPacket);
    PDECLARE_MediaReadNotifier(OpalRTPSession, OnRxControlPacket);
    void SessionFailed(SubChannels subchannel PTRACE_PARAM(, const char * reason));

    OpalRTPEndPoint   & m_endpoint;
    OpalManager       & m_manager;
    bool                m_singlePortRx;
    bool                m_singlePortTx;
    bool                m_reducedSizeRTCP;
    bool                m_isAudio;
    unsigned            m_timeUnits;
    PString             m_toolName;
    PString             m_label;
    RTPHeaderExtensions m_headerExtensions;
    unsigned            m_absSendTimeHdrExtId;
    unsigned            m_transportWideSeqNumHdrExtId;
    bool                m_allowAnySyncSource;
    PTimeInterval       m_staleReceiverTimeout;
    PINDEX              m_maxOutOfOrderPackets; // Number of packets before we give up waiting for an out of order packet
    PTimeInterval       m_waitOutOfOrderTime;   // Milliseconds before we give up on an out of order packet
    unsigned            m_txStatisticsInterval;
    unsigned            m_rxStatisticsInterval;
    OpalMediaFormat::RTCPFeedback m_feedback;

    OpalJitterBuffer  * m_jitterBuffer;

#if OPAL_RTP_FEC
    RTP_DataFrame::PayloadTypes m_redundencyPayloadType;
    RTP_DataFrame::PayloadTypes m_ulpFecPayloadType;
    unsigned                    m_ulpFecSendLevel;
#endif // OPAL_RTP_FEC

    class NotifierMap : public std::multimap<unsigned, DataNotifier>
    {
    public:
      void Add(unsigned priority, const DataNotifier & notifier);
      void Remove(const DataNotifier & notifier);
    };
    NotifierMap m_notifiers;

    friend struct SyncSource;
    struct SyncSource
    {
      SyncSource(OpalRTPSession & session, RTP_SyncSourceId id, Direction dir, const char * cname);
      virtual ~SyncSource();

#if PTRACING
      friend ostream & operator<<(ostream & strm, const SyncSource & ssrc)
      { return strm << ssrc.m_session << "SSRC=" << RTP_TRACE_SRC(ssrc.m_sourceIdentifier) << ", "; }
#endif

      virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame, RewriteMode rewrite, const PTime & now);
      virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame, ReceiveType rxType, const PTime & now);
      virtual SendReceiveStatus OnReceiveRetransmit(RTP_DataFrame & frame, const PTime & now);
      virtual void SetLastSequenceNumber(RTP_SequenceNumber sequenceNumber);
      virtual void SaveSentData(const RTP_DataFrame & frame, const PTime & now);
      virtual void OnRxNACK(const RTP_ControlFrame::LostPacketMask & lostPackets, const PTime & now);
      virtual bool IsExpectingRetransmit(RTP_SequenceNumber sequenceNumber);
      virtual SendReceiveStatus OnOutOfOrderPacket(RTP_DataFrame & frame, ReceiveType & rxType, const PTime & now);
      virtual bool HasPendingFrames() const;
      virtual bool HandlePendingFrames(const PTime & now);
#if OPAL_RTP_FEC
      virtual SendReceiveStatus OnSendRedundantFrame(RTP_DataFrame & frame);
      virtual SendReceiveStatus OnSendRedundantData(RTP_DataFrame & primary, RTP_DataFrameList & redundancies);
      virtual SendReceiveStatus OnReceiveRedundantFrame(RTP_DataFrame & frame);
      virtual SendReceiveStatus OnReceiveRedundantData(RTP_DataFrame & primary, RTP_DataFrame::PayloadTypes payloadType, unsigned timestamp, const BYTE * data, PINDEX size);
      virtual SendReceiveStatus OnSendFEC(RTP_DataFrame & primary, FecData & fec);
      virtual SendReceiveStatus OnReceiveFEC(RTP_DataFrame & primary, const FecData & fec);
#endif // OPAL_RTP_FEC


      void CalculateRTT(const PTime & reportTime, const PTimeInterval & reportDelay, const PTime & now);
      virtual void CalculateStatistics(const RTP_DataFrame & frame, const PTime & now);
#if OPAL_STATISTICS
      virtual void GetStatistics(OpalMediaStatistics & statistics) const;
#endif

      virtual bool OnSendReceiverReport(RTP_ControlFrame::ReceiverReport * report, const PTime & now PTRACE_PARAM(, unsigned logLevel));
      virtual bool OnSendDelayLastReceiverReport(RTP_ControlFrame::DelayLastReceiverReport::Receiver * report, const PTime & now);
      virtual void OnRxSenderReport(const RTP_SenderReport & report, const PTime & now);
      virtual void OnRxReceiverReport(const RTP_ReceiverReport & report, const PTime & now);
      virtual void OnRxDelayLastReceiverReport(const RTP_DelayLastReceiverReport & dlrr, const PTime & now);
      virtual SendReceiveStatus SendBYE();

      bool IsStaleReceiver(const PTime & now) const;
      bool IsRtx() const { return m_rtxPT != RTP_DataFrame::IllegalPayloadType; }

      OpalRTPSession  & m_session;
      Direction         m_direction;
      RTP_SyncSourceId  m_sourceIdentifier;
      RTP_SyncSourceId  m_loopbackIdentifier;
      PString           m_canonicalName;
      PString           m_mediaStreamId;
      PString           m_mediaTrackId;

      RTP_SyncSourceId            m_rtxSSRC; // Bidirectional link between primary and secondary
      RTP_DataFrame::PayloadTypes m_rtxPT;   // Sending rtx payload type, or receiving rtx primary payload type, only set in seconday SSRC
      int                         m_rtxPackets;
      int                         m_rtxDuplicates;

      NotifierMap m_notifiers;

      // Sequence handling
      RTP_SequenceNumber m_lastSequenceNumber;
      uint32_t           m_firstSequenceNumber;
      uint32_t           m_extendedSequenceNumber;
      unsigned           m_lastFIRSequenceNumber;
      unsigned           m_lastTSTOSequenceNumber;
      unsigned           m_consecutiveOutOfOrderPackets;
      PTime              m_consecutiveOutOfOrderEndTime;
      RTP_SequenceNumber m_nextOutOfOrderPacket;
      PTime              m_endWaitOutOfOrderTime;
      PTime              m_lateOutOfOrderAdaptTime;
      unsigned           m_lateOutOfOrderAdaptCount;
      unsigned           m_lateOutOfOrderAdaptMax;
      PTimeInterval      m_lateOutOfOrderAdaptBoost;
      PTimeInterval      m_lateOutOfOrderAdaptPeriod;
      RTP_DataFrameList  m_pendingPackets;

      // Generating real time stamping in RTP packets
      // For e_Receive, times are from last received Sender Report, or Receiver Reference Time Report
      // For e_Sender, times are from RTP_DataFrame, or synthesized from local real time.
      RTP_Timestamp      m_reportTimestamp;
      PTime              m_reportAbsoluteTime;
      bool               m_synthesizeAbsTime;

      // Handling Abs-Send-Time header extension
      uint64_t m_absSendTimeHighBits;
      uint32_t m_absSendTimeLowBits;
#if PTRACING
      unsigned m_absSendTimeLoglevel;
#endif

      // Statistics gathered
      RTP_DataFrame::PayloadTypes m_payloadType;
      PTime    m_firstPacketTime;
      unsigned m_packets;  // Note, does not include retransmits via NACK
      uint64_t m_octets;
      unsigned m_senderReports;
      atomic<unsigned> m_NACKs;
      int      m_packetsMissing;  // As per RFC
      int      m_packetsUnrecovered; // After possible recovery via NACK (only on rx)
      int      m_maxConsecutiveLost;
      unsigned m_packetsOutOfOrder;
      int      m_lateOutOfOrder;

      int      m_averagePacketTime; // Milliseconds
      int      m_maximumPacketTime; // Milliseconds
      int      m_minimumPacketTime; // Milliseconds
      int      m_currentjitter;     // Milliseconds
      int      m_maximumJitter;     // Milliseconds
      unsigned m_markerCount;

      // Working for calculating statistics
      RTP_Timestamp      m_lastPacketTimestamp;
      PTime              m_lastPacketAbsTime;
      PTime              m_lastPacketNetTime;

      unsigned           m_averageTimeAccum;
      unsigned           m_maximumTimeAccum;
      unsigned           m_minimumTimeAccum;
      unsigned           m_jitterAccum;
      RTP_Timestamp      m_lastJitterTimestamp;

      // Things to remember for filling in fields of sent SR/RR/DLRR
      unsigned           m_lastRRPacketsReceived;
      uint32_t           m_lastRRSequenceNumber;
      float              m_rtcpDiscardRate;       // from extended RR
      int                m_rtcpJitterBufferDelay; // from extended RR
      uint64_t           m_ntpPassThrough;       // The NTP time from SR
      PTime              m_lastSenderReportTime; // Local time that SR was sent/received
      PTime              m_referenceReportTime;
      PTime              m_referenceReportNTP;

      unsigned           m_statisticsCount;

#if OPAL_RTCP_XR
      RTCP_XR_Metrics  * m_metrics; // Calculate the VoIP Metrics for RTCP-XR
#endif

      OpalJitterBuffer * m_jitterBuffer;
      OpalJitterBuffer * GetJitterBuffer() const;

      PTRACE_THROTTLE(m_throttleSendData,3,20000);
      PTRACE_THROTTLE(m_throttleReceiveData,3,20000);
      PTRACE_THROTTLE(m_throttleRxSR,3,60000,5);
      PTRACE_THROTTLE(m_throttleRxRR,3,60000, 5);
      PTRACE_THROTTLE(m_throttleTxRED,3,60000);
      PTRACE_THROTTLE(m_throttleRxRED,3,60000);
      PTRACE_THROTTLE(m_throttleRxUnknownFEC,3,10000);
      PTRACE_THROTTLE(m_throttleInvalidLost,2,60000);

      P_REMOVE_VIRTUAL(SendReceiveStatus, OnSendData(RTP_DataFrame &, bool), e_AbortTransport);
      P_REMOVE_VIRTUAL(SendReceiveStatus, OnSendData(RTP_DataFrame &, RewriteMode), e_AbortTransport);
      P_REMOVE_VIRTUAL(SendReceiveStatus, OnOutOfOrderPacket(RTP_DataFrame &), e_AbortTransport);
      P_REMOVE_VIRTUAL(SendReceiveStatus, OnReceiveData(RTP_DataFrame &, ReceiveType), e_AbortTransport);
   };

    typedef std::map<RTP_SyncSourceId, SyncSource *> SyncSourceMap;
    SyncSourceMap    m_SSRC;
    SyncSource       m_dummySyncSource;
    RTP_SyncSourceId m_defaultSSRC[2]; // For e_Sender and e_Receiver

    const SyncSource & GetSyncSource(RTP_SyncSourceId ssrc, Direction dir) const;
    virtual bool GetSyncSource(RTP_SyncSourceId ssrc, Direction dir, SyncSource * & info);
    virtual SyncSource * UseSyncSource(RTP_SyncSourceId ssrc, Direction dir, bool force);
    virtual SyncSource * CreateSyncSource(RTP_SyncSourceId id, Direction dir, const char * cname);
    virtual bool CheckControlSSRC(RTP_SyncSourceId senderSSRC, RTP_SyncSourceId targetSSRC, SyncSource * & info PTRACE_PARAM(, const char * pduName));
    virtual bool ResequenceOutOfOrderPackets(SyncSource & ssrc) const;

    /// Set up RTCP as per RFC rules
    virtual bool InternalSendReport(RTP_ControlFrame & report, SyncSource & sender, bool includeReceivers, bool forced, const PTime & now);
    virtual void InitialiseControlFrame(RTP_ControlFrame & frame, SyncSource & sender);

    // Some statitsics not SSRC related
    unsigned m_rtcpPacketsSent;
    unsigned m_rtcpPacketsReceived;
    int      m_roundTripTime;

    PTimer m_reportTimer;
    PDECLARE_NOTIFIER(PTimer, OpalRTPSession, TimedSendReport);

    // Congestion control
    OpalMediaTransport::CongestionControl * GetCongestionControl();

    // Quality of service support
    PIPSocket::QoS m_qos;
    unsigned       m_packetOverhead;
    WORD           m_remoteControlPort;
    bool           m_sendEstablished;

    // Call backs for transport data
    OpalMediaTransport::ReadNotifier m_dataNotifier;
    OpalMediaTransport::ReadNotifier m_controlNotifier;

    ApplDefinedNotifierList m_applDefinedNotifiers;

    PTRACE_THROTTLE(m_throttleTxReport,3,60000,5);
    PTRACE_THROTTLE(m_throttleRxEmptyRR,3,60000);
    PTRACE_THROTTLE(m_throttleRxSDES,4,60000);
#if PTRACING
    std::set<RTP_SyncSourceId> m_loggedBadSSRC;
#endif

  private:
    OpalRTPSession(const OpalRTPSession &);
    void operator=(const OpalRTPSession &) { }

    P_REMOVE_VIRTUAL(int,WaitForPDU(PUDPSocket&,PUDPSocket&,const PTimeInterval&),0);
    P_REMOVE_VIRTUAL(SendReceiveStatus,ReadDataOrControlPDU(BYTE *,PINDEX,bool),e_AbortTransport);
    P_REMOVE_VIRTUAL(bool,WriteDataOrControlPDU(const BYTE *,PINDEX,bool),false);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnSendData(RTP_DataFrame &),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnSendData(RTP_DataFrame&,bool),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnSendData(RTP_DataFrame&,RewriteMode),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnSendData(RTP_DataFrame&,RewriteMode,const PTime&),e_AbortTransport);
    P_REMOVE_VIRTUAL(bool,WriteData(RTP_DataFrame &,const PIPSocketAddressAndPort*,bool),false);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnReadTimeout(RTP_DataFrame&),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,InternalReadData(RTP_DataFrame &),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,SendReport(bool),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnReceiveData(RTP_DataFrame&, PINDEX),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnReceiveData(RTP_DataFrame&),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnOutOfOrderPacket(RTP_DataFrame&),e_AbortTransport);
    P_REMOVE_VIRTUAL(SendReceiveStatus,OnPreReceiveData(RTP_DataFrame &),e_AbortTransport);

  friend class RTCP_XR_Metrics;
};


#endif // OPAL_RTP_RTP_H

/////////////////////////////////////////////////////////////////////////////
