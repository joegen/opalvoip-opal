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

#include <opal/buildopts.h>

#include <rtp/rtp.h>
#include <opal/mediasession.h>
#include <ptlib/sockets.h>
#include <ptlib/safecoll.h>
#include <ptclib/pnat.h>
#include <ptclib/url.h>

#include <list>


class OpalJitterBuffer;
class PNatMethod;
class RTCP_XR_Metrics;


///////////////////////////////////////////////////////////////////////////////
// 
// class to hold the QoS definitions for an RTP channel

class RTP_QOS : public PObject
{
  PCLASSINFO(RTP_QOS,PObject);
  public:
    PQoS dataQoS;
    PQoS ctrlQoS;
};


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

    virtual void AttachTransport(Transport & transport);
    virtual Transport DetachTransport();

    virtual void SetExternalTransport(const OpalTransportAddressArray & transports);

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      bool isSource
    );
  //@}

  /**@name Operations */
  //@{
    /**Sets the size of the jitter buffer to be used by this RTP session.
       A session defaults to not having any jitter buffer enabled for reading
       and the ReadBufferedData() function simply calls ReadData().

       If either jitter delay parameter is zero, it destroys the jitter buffer
       attached to this RTP session.
      */
    void SetJitterBufferSize(
      unsigned minJitterDelay, ///<  Minimum jitter buffer delay in RTP timestamp units
      unsigned maxJitterDelay, ///<  Maximum jitter buffer delay in RTP timestamp units
      unsigned timeUnits = 0,  ///<  Time Units, zero uses default
      PINDEX packetSize = 2048 ///<  Receive RTP packet size
    );

    /**Get current size of the jitter buffer.
       This returns the currently used jitter buffer delay in RTP timestamp
       units. It will be some value between the minimum and maximum set in
       the SetJitterBufferSize() function.
      */
    unsigned GetJitterBufferSize() const;
    unsigned GetJitterBufferDelay() const { return GetJitterBufferSize()/GetJitterTimeUnits(); }
    
    /**Get current time units of the jitter buffer.
     */
    unsigned GetJitterTimeUnits() const { return m_timeUnits; }

#if P_QOS
    /**Modifies the QOS specifications for this RTP session*/
    virtual bool ModifyQOS(RTP_QOS *);
#endif

    /**Read a data frame from the RTP channel.
       This function will conditionally read data from the jitter buffer or
       directly if there is no jitter buffer enabled.
      */
    virtual bool ReadData(
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
    );

    /**Flush incoming data.
      */
    virtual void FlushData();

    /**Write a data frame from the RTP channel.
      */
    virtual bool WriteData(
      RTP_DataFrame & frame   ///<  Frame to write to the RTP session
    );

    /** Write data frame to the RTP channel outside the normal stream of media
      * Used for RFC2833 packets
      */
    virtual bool WriteOOBData(
      RTP_DataFrame & frame,
      bool rewriteTimeStamp = true
    );

    /**Write a control frame from the RTP channel.
      */
    virtual bool WriteControl(
      RTP_ControlFrame & frame    ///<  Frame to write to the RTP session
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
    virtual void GetStatistics(OpalMediaStatistics & statistics, bool receiver) const;
#endif
  //@}

  /**@name Call back functions */
  //@{
    enum SendReceiveStatus {
      e_ProcessPacket,
      e_IgnorePacket,
      e_AbortTransport
    };
    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);

    class ReceiverReport : public PObject  {
        PCLASSINFO(ReceiverReport, PObject);
      public:
        void PrintOn(ostream &) const;

        DWORD sourceIdentifier;
        DWORD fractionLost;         /* fraction lost since last SR/RR */
        DWORD totalLost;	    /* cumulative number of packets lost (signed!) */
        DWORD lastSequenceNumber;   /* extended last sequence number received */
        DWORD jitter;               /* interarrival jitter */
        PTimeInterval lastTimestamp;/* last SR packet from this source */
        PTimeInterval delay;        /* delay since last SR packet */
    };
    PARRAY(ReceiverReportArray, ReceiverReport);

    class SenderReport : public PObject  {
        PCLASSINFO(SenderReport, PObject);
      public:
        void PrintOn(ostream &) const;

        DWORD sourceIdentifier;
        PTime realTimestamp;
        DWORD rtpTimestamp;
        DWORD packetsSent;
        DWORD octetsSent;
    };

    virtual void OnRxSenderReport(const SenderReport & sender,
                                  const ReceiverReportArray & reports);
    virtual void OnRxReceiverReport(DWORD src,
                                    const ReceiverReportArray & reports);
    virtual void OnReceiverReports(const ReceiverReportArray & reports);

    class SourceDescription : public PObject  {
        PCLASSINFO(SourceDescription, PObject);
      public:
        SourceDescription(DWORD src) { sourceIdentifier = src; }
        void PrintOn(ostream &) const;

        DWORD            sourceIdentifier;
        POrdinalToString items;
    };
    PARRAY(SourceDescriptionArray, SourceDescription);
    virtual void OnRxSourceDescription(const SourceDescriptionArray & descriptions);

    virtual void OnRxGoodbye(const PDWORDArray & sources,
                             const PString & reason);

    virtual void OnRxApplDefined(const PString & type, unsigned subtype, DWORD src,
                                 const BYTE * data, PINDEX size);

#if OPAL_RTCP_XR
    class ExtendedReport : public PObject  {
        PCLASSINFO(ExtendedReport, PObject);
      public:
        void PrintOn(ostream &) const;

        DWORD sourceIdentifier;
        DWORD lossRate;            /* fraction of RTP data packets lost */ 
        DWORD discardRate;         /* fraction of RTP data packets discarded */
        DWORD burstDensity;        /* fraction of RTP data packets within burst periods */
        DWORD gapDensity;          /* fraction of RTP data packets within inter-burst gaps */
        DWORD roundTripDelay;  /* the most recently calculated round trip time */    
        DWORD RFactor;            /* voice quality metric of the call */
        DWORD mosLQ;               /* MOS for listen quality */
        DWORD mosCQ;               /* MOS for conversational quality */
        DWORD jbNominal;      /* current nominal jitter buffer delay, in ms */ 
        DWORD jbMaximum;      /* current maximum jitter buffer delay, in ms */
        DWORD jbAbsolute;     /* current absolute maximum jitter buffer delay, in ms */
    };
    PARRAY(ExtendedReportArray, ExtendedReport);

    virtual void OnRxExtendedReport(
      DWORD src,
      const ExtendedReportArray & reports
    );

    RTCP_XR_Metrics * GetExtendedMetrics() const { return m_metrics; }
#endif // OPAL_RTCP_XR
  //@}

  /**@name Member variable access */
  //@{
    /**Get flag for is audio RTP.
      */
    bool IsAudio() const { return m_isAudio; }

    /**Set flag for RTP session is audio.
     */
    void SetAudio(
      bool aud    /// New audio indication flag
    ) { m_isAudio = aud; }

    /**Get the canonical name for the RTP session.
      */
    PString GetCanonicalName() const;

    /**Set the canonical name for the RTP session.
      */
    void SetCanonicalName(const PString & name);

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
    DWORD GetSyncSourceOut() const { return syncSourceOut; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    bool AllowAnySyncSource() const { return allowAnySyncSource; }

    /**Indicate if will ignore all but first received SSRC value.
      */
    void SetAnySyncSource(
      bool allow    ///<  Flag for allow any SSRC values
    ) { allowAnySyncSource = allow; }

    /**Indicate if will ignore rtp payload type changes in received packets.
     */
    void SetIgnorePayloadTypeChanges(
      bool ignore   ///<  Flag to ignore payload type changes
    ) { ignorePayloadTypeChanges = ignore; }

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

    /**Get the time interval for sending RTCP reports in the session.
      */
    const PTimeInterval & GetReportTimeInterval() { return m_reportTimer.GetResetTime(); }

    /**Set the time interval for sending RTCP reports in the session.
      */
    void SetReportTimeInterval(
      const PTimeInterval & interval ///<  New time interval for reports.
    )  { m_reportTimer.RunContinuous(interval); }

    /**Get the interval for transmitter statistics in the session.
      */
    unsigned GetTxStatisticsInterval() { return txStatisticsInterval; }

    /**Set the interval for transmitter statistics in the session.
      */
    void SetTxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Get the interval for receiver statistics in the session.
      */
    unsigned GetRxStatisticsInterval() { return rxStatisticsInterval; }

    /**Set the interval for receiver statistics in the session.
      */
    void SetRxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Clear statistics
      */
    void ClearStatistics();

    /**Get local data port of session.
      */
    virtual WORD GetLocalDataPort() const { return m_localDataPort; }

    /**Get local control port of session.
      */
    virtual WORD GetLocalControlPort() const { return m_localControlPort; }

    /**Get remote data port of session.
      */
    virtual WORD GetRemoteDataPort() const { return m_remoteDataPort; }

    /**Get remote control port of session.
      */
    virtual WORD GetRemoteControlPort() const { return m_remoteControlPort; }

    /**Get data UDP socket of session.
      */
    virtual PUDPSocket & GetDataSocket() { return *m_dataSocket; }

    /**Get control UDP socket of session.
      */
    virtual PUDPSocket & GetControlSocket() { return *m_controlSocket; }

    /**Get total number of packets sent in session.
      */
    DWORD GetPacketsSent() const { return packetsSent; }

    /**Get total number of octets sent in session.
      */
    DWORD GetOctetsSent() const { return octetsSent; }

    /**Get total number of packets received in session.
      */
    DWORD GetPacketsReceived() const { return packetsReceived; }

    /**Get total number of octets received in session.
      */
    DWORD GetOctetsReceived() const { return octetsReceived; }

    /**Get total number received packets lost in session.
      */
    DWORD GetPacketsLost() const { return packetsLost; }

    /**Get total number transmitted packets lost by remote in session.
       Determined via RTCP.
      */
    DWORD GetPacketsLostByRemote() const { return packetsLostByRemote; }

    /**Get total number of packets received out of order in session.
      */
    DWORD GetPacketsOutOfOrder() const { return packetsOutOfOrder; }

    /**Get total number received packets too late to go into jitter buffer.
      */
    DWORD GetPacketsTooLate() const;

    /**Get total number received packets that could not fit into the jitter buffer.
      */
    DWORD GetPacketOverruns() const;

    /**Get average time between sent packets.
       This is averaged over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetAverageSendTime() const { return averageSendTime; }

    /**Get the number of marker packets received this session.
       This can be used to find out the number of frames received in a video
       RTP stream.
      */
    DWORD GetMarkerRecvCount() const { return markerRecvCount; }

    /**Get the number of marker packets sent this session.
       This can be used to find out the number of frames sent in a video
       RTP stream.
      */
    DWORD GetMarkerSendCount() const { return markerSendCount; }

    /**Get maximum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMaximumSendTime() const { return maximumSendTime; }

    /**Get minimum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMinimumSendTime() const { return minimumSendTime; }

    /**Get average time between received packets.
       This is averaged over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetAverageReceiveTime() const { return averageReceiveTime; }

    /**Get maximum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMaximumReceiveTime() const { return maximumReceiveTime; }

    /**Get minimum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMinimumReceiveTime() const { return minimumReceiveTime; }

    enum { JitterRoundingGuardBits = 4 };
    /**Get averaged jitter time for received packets.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetAvgJitterTime() const { return (jitterLevel>>JitterRoundingGuardBits)/GetJitterTimeUnits(); }

    /**Get averaged jitter time for received packets.
       This is the maximum value of jitterLevel for the session.
      */
    DWORD GetMaxJitterTime() const { return (maximumJitterLevel>>JitterRoundingGuardBits)/GetJitterTimeUnits(); }

    /**Get jitter time for received packets on remote.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetJitterTimeOnRemote() const { return jitterLevelOnRemote/GetJitterTimeUnits(); }
  //@}

    virtual void SetCloseOnBYE(bool v)  { m_closeOnBye = v; }

    /**Send flow control (Temporary Maximum Media Stream Bit Rate) Request/Notification.
      */
    virtual void SendFlowControl(
      unsigned maxBitRate,    ///< New temporary maximum bit rate
      unsigned overhead = 0,  ///< Protocol overhead, defaults to IP/UDP/RTP header size
      bool notify = false     ///< Send request/notification
    );

#if OPAL_VIDEO
    /** Tell the rtp session to send out an intra frame request control packet.
        This is called when the media stream receives an OpalVideoUpdatePicture
        media command.
      */
    virtual void SendIntraFrameRequest(bool rfc2032, bool pictureLoss);

    /** Tell the rtp session to send out an temporal spatial trade off request
        control packet. This is called when the media stream receives an
        OpalTemporalSpatialTradeOff media command.
      */
    virtual void SendTemporalSpatialTradeOff(unsigned tradeOff);
#endif

    void SetNextSentSequenceNumber(WORD num) { lastSentSequenceNumber = (WORD)(num-1); }

    DWORD GetSyncSourceIn() const { return syncSourceIn; }

    typedef PNotifierTemplate<SendReceiveStatus &> FilterNotifier;
    #define PDECLARE_RTPFilterNotifier(cls, fn) PDECLARE_NOTIFIER2(RTP_DataFrame, cls, fn, OpalRTPSession::SendReceiveStatus &)
    #define PCREATE_RTPFilterNotifier(fn) PCREATE_NOTIFIER2(fn, OpalRTPSession::SendReceiveStatus &)

    void AddFilter(const FilterNotifier & filter);

    virtual void SendBYE();

  protected:
    ReceiverReportArray BuildReceiverReportArray(const RTP_ControlFrame & frame, PINDEX offset);
    void AddReceiverReport(RTP_ControlFrame::ReceiverReport & receiver);
    bool InsertReportPacket(RTP_ControlFrame & report);
    virtual int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timer);
    virtual SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame);
    
    virtual void ApplyQOS(const PIPSocket::Address & addr);
    virtual bool InternalReadData(RTP_DataFrame & frame);
    virtual SendReceiveStatus ReadControlPDU();
    virtual SendReceiveStatus ReadDataOrControlPDU(
      BYTE * framePtr,
      PINDEX frameSize,
      bool fromDataChannel
    );

    virtual bool WriteDataOrControlPDU(
      const BYTE * framePtr,
      PINDEX frameSize,
      bool toDataChannel
    );


    bool                m_isAudio;
    unsigned            m_timeUnits;
    PString             m_canonicalName;
    PString             m_toolName;
    RTPExtensionHeaders m_extensionHeaders;
    PTimeInterval       m_maxNoReceiveTime;
    PTimeInterval       m_maxNoTransmitTime;

    DWORD         syncSourceOut;
    DWORD         syncSourceIn;
    DWORD         lastSentTimestamp;
    bool          allowAnySyncSource;
    bool          allowOneSyncSourceChange;
    bool          allowRemoteTransmitAddressChange;
    bool          allowSequenceChange;
    unsigned      txStatisticsInterval;
    unsigned      rxStatisticsInterval;
    WORD          lastSentSequenceNumber;
    WORD          expectedSequenceNumber;
    PTimeInterval lastSentPacketTime;
    PTimeInterval lastReceivedPacketTime;
    PTime         lastSRTimestamp;
    PTime         lastSRReceiveTime;
    PTimeInterval delaySinceLastSR;
    WORD          lastRRSequenceNumber;
    bool          resequenceOutOfOrderPackets;
    unsigned      consecutiveOutOfOrderPackets;
    PTimeInterval outOfOrderWaitTime;
    PTimeInterval outOfOrderPacketTime;

    std::list<RTP_DataFrame> m_outOfOrderPackets;
    void SaveOutOfOrderPacket(RTP_DataFrame & frame);

    DWORD         timeStampOffs;               // offset between incoming media timestamp and timeStampOut
    bool          oobTimeStampBaseEstablished; // true if timeStampOffs has been established by media
    DWORD         oobTimeStampOutBase;         // base timestamp value for oob data
    PTimeInterval oobTimeStampBase;            // base time for oob timestamp

    // Statistics
    PTime firstPacketSent;
    DWORD packetsSent;
    DWORD rtcpPacketsSent;
    DWORD octetsSent;
    PTime firstPacketReceived;
    DWORD packetsReceived;
    DWORD senderReportsReceived;
    DWORD octetsReceived;
    DWORD packetsLost;
    DWORD packetsLostByRemote;
    DWORD packetsOutOfOrder;
    DWORD averageSendTime;
    DWORD maximumSendTime;
    DWORD minimumSendTime;
    DWORD averageReceiveTime;
    DWORD maximumReceiveTime;
    DWORD minimumReceiveTime;
    DWORD jitterLevel;
    DWORD jitterLevelOnRemote;
    DWORD maximumJitterLevel;

    DWORD m_syncTimestamp;
    PTime m_syncRealTime;

    DWORD markerSendCount;
    DWORD markerRecvCount;

    unsigned txStatisticsCount;
    unsigned rxStatisticsCount;
    
#if OPAL_RTCP_XR
    // Calculate the VoIP Metrics for RTCP-XR
    RTCP_XR_Metrics * m_metrics;
    friend class RTCP_XR_Metrics;
#endif

    DWORD    averageSendTimeAccum;
    DWORD    maximumSendTimeAccum;
    DWORD    minimumSendTimeAccum;
    DWORD    averageReceiveTimeAccum;
    DWORD    maximumReceiveTimeAccum;
    DWORD    minimumReceiveTimeAccum;
    DWORD    packetsLostSinceLastRR;
    DWORD    lastTransitTime;
    DWORD    m_lastReceivedStatisticTimestamp;
    
    RTP_DataFrame::PayloadTypes lastReceivedPayloadType;
    bool ignorePayloadTypeChanges;

    PMutex m_reportMutex;
    PTimer m_reportTimer;
    PDECLARE_NOTIFIER(PTimer, OpalRTPSession, SendReport);

    PMutex m_dataMutex;
    bool   m_closeOnBye;
    bool   m_byeSent;

    list<FilterNotifier> m_filters;

    PIPSocket::Address m_localAddress;
    WORD               m_localDataPort;
    WORD               m_localControlPort;

    PIPSocket::Address m_remoteAddress;
    WORD               m_remoteDataPort;
    WORD               m_remoteControlPort;

    PIPSocket::Address m_remoteTransmitAddress;

    PUDPSocket * m_dataSocket;
    PUDPSocket * m_controlSocket;

    bool m_shutdownRead;
    bool m_shutdownWrite;
    bool m_appliedQOS;
    bool m_remoteBehindNAT;
    bool m_localHasRestrictedNAT;
    bool m_firstControl;

    DWORD        m_noTransmitErrors;
    PSimpleTimer m_noTransmitTimer;

    // Make sure JB is last to make sure it is destroyed first.
    typedef PSafePtr<OpalJitterBuffer, PSafePtrMultiThreaded> JitterBufferPtr;
    JitterBufferPtr m_jitterBuffer;

  private:
    OpalRTPSession(const OpalRTPSession &);
    void operator=(const OpalRTPSession &) { }

  friend class RTP_JitterBuffer;
};


#endif // OPAL_RTP_RTP_H

/////////////////////////////////////////////////////////////////////////////
