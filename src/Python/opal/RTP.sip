/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <rtp/rtp.h>
%End


///////////////////////////////////////////////////////////////////////////////
// 
// class to hold the QoS definitions for an RTP channel

class RTP_QOS : PObject
{
  public:
    PQoS dataQoS;
    PQoS ctrlQoS;
};


///////////////////////////////////////////////////////////////////////////////
// Real Time Protocol - IETF RFC1889 and RFC1890

/**An RTP data frame encapsulation.
  */
class RTP_DataFrame
{

  public:
    RTP_DataFrame(PINDEX payloadSize = 0, PINDEX bufferSize = 0);
    RTP_DataFrame(const BYTE * data, PINDEX len, PBoolean dynamic = true);

    enum {
      ProtocolVersion = 2,
      MinHeaderSize = 12,
      // Max safe MTU size (576 bytes as per RFC879) minus IP, UDP an RTP headers
      MaxMtuPayloadSize = 528 //(576-20-16-12)
    };

    enum PayloadTypes {
      PCMU,         // G.711 u-Law
      FS1016,       // Federal Standard 1016 CELP
      G721,         // ADPCM - Subsumed by G.726
      G726 = G721,
      GSM,          // GSM 06.10
      G7231,        // G.723.1 at 6.3kbps or 5.3 kbps
      DVI4_8k,      // DVI4 at 8kHz sample rate
      DVI4_16k,     // DVI4 at 16kHz sample rate
      LPC,          // LPC-10 Linear Predictive CELP
      PCMA,         // G.711 A-Law
      G722,         // G.722
      L16_Stereo,   // 16 bit linear PCM
      L16_Mono,     // 16 bit linear PCM
      G723,         // G.723
      CN,           // Confort Noise
      MPA,          // MPEG1 or MPEG2 audio
      G728,         // G.728 16kbps CELP
      DVI4_11k,     // DVI4 at 11kHz sample rate
      DVI4_22k,     // DVI4 at 22kHz sample rate
      G729,         // G.729 8kbps
      Cisco_CN,     // Cisco systems comfort noise (unofficial)

      CelB = 25,    // Sun Systems Cell-B video
      JPEG,         // Motion JPEG
      H261 = 31,    // H.261
      MPV,          // MPEG1 or MPEG2 video
      MP2T,         // MPEG2 transport system
      H263,         // H.263

      T38 = 38,     // T.38 (internal)

      LastKnownPayloadType,

      DynamicBase = 96,
      MaxPayloadType = 127,
      IllegalPayloadType
    };

    unsigned GetVersion() const;

    PBoolean GetExtension() const;
    void SetExtension(PBoolean ext);

    PBoolean GetMarker() const;
    void SetMarker(PBoolean m);

    bool GetPadding() const;
    void SetPadding(bool v);
    BYTE * GetPaddingPtr() const;

    unsigned GetPaddingSize() const;
    bool     SetPaddingSize(PINDEX sz);

    PayloadTypes GetPayloadType() const;
    void         SetPayloadType(PayloadTypes t);

    WORD GetSequenceNumber() const;
    void SetSequenceNumber(WORD n);

    DWORD GetTimestamp() const;
    void  SetTimestamp(DWORD t);

    DWORD GetSyncSource() const;
    void  SetSyncSource(DWORD s);

    PINDEX GetContribSrcCount() const;
    DWORD  GetContribSource(PINDEX idx) const;
    void   SetContribSource(PINDEX idx, DWORD src);

    PINDEX GetHeaderSize() const;

    int GetExtensionType() const; // -1 is no extension
    void   SetExtensionType(int type);
    PINDEX GetExtensionSizeDWORDs() const;      // get the number of 32 bit words in the extension (excluding the header).
    bool   SetExtensionSizeDWORDs(PINDEX sz);   // set the number of 32 bit words in the extension (excluding the header)
    BYTE * GetExtensionPtr() const;

    PINDEX GetPayloadSize() const;
    bool   SetPayloadSize(PINDEX sz);
    BYTE * GetPayloadPtr()     const;


    // Note this sets the whole packet length, and calculates the various
    // sub-section sizes: header payload and padding.
    bool SetPacketSize(PINDEX sz);
};

typedef PList<RTP_DataFrame> RTP_DataFrameList;


/**An RTP control frame encapsulation.
  */
class RTP_ControlFrame
{
  public:
    RTP_ControlFrame(PINDEX compoundSize = 2048);

    unsigned GetVersion() const;

    unsigned GetCount() const;
    void     SetCount(unsigned count);

    enum PayloadTypes {
      e_IntraFrameRequest       = 192,
      e_SenderReport            = 200,
      e_ReceiverReport          = 201,
      e_SourceDescription       = 202,
      e_Goodbye                 = 203,
      e_ApplDefined             = 204,
      e_TransportLayerFeedBack  = 205, // RFC4585
      e_PayloadSpecificFeedBack = 206,
      e_ExtendedReport          = 207  // RFC3611
    };

    unsigned GetPayloadType() const;
    void     SetPayloadType(unsigned t);

    PINDEX GetPayloadSize() const;
    void   SetPayloadSize(PINDEX sz);

    BYTE * GetPayloadPtr() const;

    PBoolean ReadNextPacket();
    PBoolean StartNewPacket();
    void EndPacket();

    PINDEX GetCompoundSize() const;

    void Reset(PINDEX size);

// #pragma pack(1)
    // struct ReceiverReport {
      // PUInt32b ssrc;      /* data source being reported */
      // BYTE fraction;      /* fraction lost since last SR/RR */
      // BYTE lost[3];	  /* cumulative number of packets lost (signed!) */
      // PUInt32b last_seq;  /* extended last sequence number received */
      // PUInt32b jitter;    /* interarrival jitter */
      // PUInt32b lsr;       /* last SR packet from this source */
      // PUInt32b dlsr;      /* delay since last SR packet */

      // unsigned GetLostPackets() const { return (lost[0]<<16U)+(lost[1]<<8U)+lost[2]; }
      // void SetLostPackets(unsigned lost);
    // };

    // struct SenderReport {
      // PUInt32b ntp_sec;   /* NTP timestamp */
      // PUInt32b ntp_frac;
      // PUInt32b rtp_ts;    /* RTP timestamp */
      // PUInt32b psent;     /* packets sent */
      // PUInt32b osent;     /* octets sent */ 
    // };

    // enum DescriptionTypes {
      // e_END,
      // e_CNAME,
      // e_NAME,
      // e_EMAIL,
      // e_PHONE,
      // e_LOC,
      // e_TOOL,
      // e_NOTE,
      // e_PRIV,
      // NumDescriptionTypes
    // };

    // struct SourceDescription {
      // PUInt32b src;       /* first SSRC/CSRC */
      // struct Item {
        // BYTE type;        /* type of SDES item (enum DescriptionTypes) */
        // BYTE length;      /* length of SDES item (in octets) */
        // char data[1];     /* text, not zero-terminated */

        // /* WARNING, SourceDescription may not be big enough to contain length and data, for 
           // instance, when type == RTP_ControlFrame::e_END.
           // Be careful whan calling the following function of it may read to over to 
           // memory allocated*/
        // unsigned int GetLengthTotal() const {return (unsigned int)(length + 2);} 
        // const Item * GetNextItem() const { return (const Item *)((char *)this + length + 2); }
        // Item * GetNextItem() { return (Item *)((char *)this + length + 2); }
      // } item[1];          /* list of SDES items */
    // };

    // void StartSourceDescription(
      // DWORD src   ///<  SSRC/CSRC identifier
    // );

    // void AddSourceDescriptionItem(
      // unsigned type,            ///<  Description type
      // const PString & data      ///<  Data for description
    // );
// #pragma pack()

};



///////////////////////////////////////////////////////////////////////////////


/**This class carries statistics on the media stream.
  */
class OpalMediaStatistics : PObject
{
  public:
    OpalMediaStatistics();

    // General info (typically from RTP)
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
    };
	
	Fax m_fax;
};




/**This class is the base for user data that may be attached to the RTP_session
   allowing callbacks for statistics and progress monitoring to be passed to an
   arbitrary object that an RTP consumer may require.
  */
class RTP_UserData : PObject
{
  public:
    /**Callback from the RTP session for transmit statistics monitoring.
       This is called every RTP_Session::txStatisticsInterval packets on the
       transmitter indicating that the statistics have been updated.

       The default behaviour does nothing.
      */
    virtual void OnTxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session for receive statistics monitoring.
       This is called every RTP_Session::receiverReportInterval packets on the
       receiver indicating that the statistics have been updated.

       The default behaviour does nothing.
      */
    virtual void OnRxStatistics(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session when an intra frame request control
       packet is sent.

       The default behaviour does nothing.
      */
    virtual void OnTxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session when an intra frame request control
       packet is received.

       The default behaviour does nothing.
      */
    virtual void OnRxIntraFrameRequest(
      const RTP_Session & session   ///<  Session with statistics
    ) const;

    /**Callback from the RTP session when RTP session is failing due to the remote being unavailable
       The default behaviour does nothing.
      */
    virtual void SessionFailing(
      RTP_Session & session   ///<  Session with statistics
    );
};


/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class RTP_Session : PObject /Abstract/
{
%ConvertToSubClassCode
	sipType = sipType_RTP_UDP; // Can't really have RTP over TCP; atleast not on our network.
%End
	
  public:
  /**@name Construction */
  //@{
    struct Params {
      Params();

      PString             encoding;    ///<  identifies initial RTP encoding (RTP/AVP, UDPTL etc)
      unsigned            id;          ///<  Session ID for RTP channel
      RTP_UserData      * userData;    ///<  Optional data for session.
      bool                autoDelete;  ///<  Delete optional data with session.
      bool                isAudio;     ///<  is audio RTP data
      bool                remoteIsNAT; ///<  Remote is behid NAT
    };

    /**Create a new RTP session.
     */
    RTP_Session(
      const RTP_Session::Params & options ///< Parameters to construct with session.
    );

    /**Delete a session.
       This deletes the userData field if autoDeleteUserData is true.
     */
    ~RTP_Session();
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
    unsigned GetJitterBufferDelay() const;
    
    /**Get current time units of the jitter buffer.
     */
    unsigned GetJitterTimeUnits() const;

    /**Modifies the QOS specifications for this RTP session*/
    virtual PBoolean ModifyQOS(RTP_QOS * );

    /**Read a data frame from the RTP channel.
       This function will conditionally read data from the jitter buffer or
       directly if there is no jitter buffer enabled. An application should
       generally use this in preference to directly calling ReadData().
      */
    virtual PBoolean ReadBufferedData(
      RTP_DataFrame & frame   ///<  Frame read from the RTP session
    );

    /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual PBoolean ReadData(
      RTP_DataFrame & frame  ///<  Frame read from the RTP session
    ) = 0;

    /**Write a data frame from the RTP channel.
      */
    virtual PBoolean WriteData(
      RTP_DataFrame & frame   ///<  Frame to write to the RTP session
    ) = 0;

    /** Write data frame to the RTP channel outside the normal stream of media
      * Used for RFC2833 packets
      */
    virtual PBoolean WriteOOBData(
      RTP_DataFrame & frame,
      bool rewriteTimeStamp = true
    );

    /**Write a control frame from the RTP channel.
      */
    virtual PBoolean WriteControl(
      RTP_ControlFrame & frame    ///<  Frame to write to the RTP session
    ) = 0;

    /**Write the RTCP reports.
      */
    virtual PBoolean SendReport();

    /**Close down the RTP session.
      */
    virtual bool Close(
      PBoolean reading    ///<  Closing the read side of the session
    ) = 0;

   /**Reopens an existing session in the given direction.
      */
    virtual void Reopen(
      PBoolean isReading
    ) = 0;

    /**Get the local host name as used in SDES packes.
      */
    virtual PString GetLocalHostName() = 0;

    virtual void GetStatistics(OpalMediaStatistics & statistics, bool receiver) const;
  //@}

  /**@name Call back functions */
  //@{
    enum SendReceiveStatus {
      e_ProcessPacket,
      e_IgnorePacket,
      e_AbortTransport
    };
    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual SendReceiveStatus Internal_OnSendData(RTP_DataFrame & frame);

    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    virtual SendReceiveStatus Internal_OnSendControl(RTP_ControlFrame & frame, PINDEX & len);

    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual SendReceiveStatus Internal_OnReceiveData(RTP_DataFrame & frame);

    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);

    class ReceiverReport : PObject  {
      public:

        DWORD sourceIdentifier;
        DWORD fractionLost;         /* fraction lost since last SR/RR */
        DWORD totalLost;	    /* cumulative number of packets lost (signed!) */
        DWORD lastSequenceNumber;   /* extended last sequence number received */
        DWORD jitter;               /* interarrival jitter */
        PTimeInterval lastTimestamp;/* last SR packet from this source */
        PTimeInterval delay;        /* delay since last SR packet */
    };
	typedef PArray<RTP_Session::ReceiverReport> ReceiverReportArray;

    class SenderReport : PObject  {
      public:

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

    class SourceDescription : PObject  {
      public:
        SourceDescription(DWORD src);

        DWORD            sourceIdentifier;
        /*POrdinalToString items;*/
    };
	typedef PArray<RTP_Session::SourceDescription> SourceDescriptionArray;
    virtual void OnRxSourceDescription(const SourceDescriptionArray & descriptions);

    virtual void OnRxGoodbye(const PDWORDArray & sources,
                             const PString & reason);

    virtual void OnRxApplDefined(const PString & type, unsigned subtype, DWORD src,
                                 const BYTE * data, PINDEX size);
  //@}
  
   class ExtendedReport : PObject  {
      public:

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
	typedef PArray<RTP_Session::ExtendedReport> ExtendedReportArray;

    virtual void OnRxExtendedReport(DWORD src,
                                    const ExtendedReportArray & reports);

  /**@name Member variable access */
  //@{
    /**Get the ID for the RTP session.
      */
    unsigned GetSessionID() const;

    /**Set the ID for the RTP session.
      */
    void SetSessionID(unsigned id);

    /**Get flag for is audio RTP.
      */
    bool IsAudio() const;

    /**Set flag for RTP session is audio.
     */
    void SetAudio(
      bool aud    /// New audio indication flag
    );

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

    /**Get the user data for the session.
      */
    RTP_UserData * GetUserData() const;

    /**Set the user data for the session.
      */
    void SetUserData(
      RTP_UserData * data,            ///<  New user data to be used
      PBoolean autoDeleteUserData = true  ///<  Delete optional data with session.
    );

    /**Get the source output identifier.
      */
    DWORD GetSyncSourceOut() const;

    /**Indicate if will ignore all but first received SSRC value.
      */
    bool AllowAnySyncSource() const;

    /**Indicate if will ignore all but first received SSRC value.
      */
    void SetAnySyncSource(
      bool allow    ///<  Flag for allow any SSRC values
    );

    /**Indicate if will ignore rtp payload type changes in received packets.
     */
    void SetIgnorePayloadTypeChanges(
      PBoolean ignore   ///<  Flag to ignore payload type changes
    );

    /**Get the time interval for sending RTCP reports in the session.
      */
    const PTimeInterval & GetReportTimeInterval();

    /**Set the time interval for sending RTCP reports in the session.
      */
    void SetReportTimeInterval(
      const PTimeInterval & interval ///<  New time interval for reports.
    );

    /**Get the current report timer
     */
    PTimeInterval GetReportTimer();

    /**Get the interval for transmitter statistics in the session.
      */
    unsigned GetTxStatisticsInterval();

    /**Set the interval for transmitter statistics in the session.
      */
    void SetTxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Get the interval for receiver statistics in the session.
      */
    unsigned GetRxStatisticsInterval();

    /**Set the interval for receiver statistics in the session.
      */
    void SetRxStatisticsInterval(
      unsigned packets   ///<  Number of packets between callbacks
    );

    /**Clear statistics
      */
    void ClearStatistics();

    /**Get total number of packets sent in session.
      */
    DWORD GetPacketsSent() const;

    /**Get total number of octets sent in session.
      */
    DWORD GetOctetsSent() const;

    /**Get total number of packets received in session.
      */
    DWORD GetPacketsReceived() const;

    /**Get total number of octets received in session.
      */
    DWORD GetOctetsReceived() const;

    /**Get total number received packets lost in session.
      */
    DWORD GetPacketsLost() const;

    /**Get total number transmitted packets lost by remote in session.
       Determined via RTCP.
      */
    DWORD GetPacketsLostByRemote() const;

    /**Get total number of packets received out of order in session.
      */
    DWORD GetPacketsOutOfOrder() const;

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
    DWORD GetAverageSendTime() const;

    /**Get the number of marker packets received this session.
       This can be used to find out the number of frames received in a video
       RTP stream.
      */
    DWORD GetMarkerRecvCount() const;

    /**Get the number of marker packets sent this session.
       This can be used to find out the number of frames sent in a video
       RTP stream.
      */
    DWORD GetMarkerSendCount() const;

    /**Get maximum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMaximumSendTime() const;

    /**Get minimum time between sent packets.
       This is over the last txStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMinimumSendTime() const;

    /**Get average time between received packets.
       This is averaged over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetAverageReceiveTime() const;

    /**Get maximum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMaximumReceiveTime() const;

    /**Get minimum time between received packets.
       This is over the last rxStatisticsInterval packets and is in
       milliseconds.
      */
    DWORD GetMinimumReceiveTime() const;

    enum { JitterRoundingGuardBits = 4 };
    /**Get averaged jitter time for received packets.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetAvgJitterTime() const;

    /**Get averaged jitter time for received packets.
       This is the maximum value of jitterLevel for the session.
      */
    DWORD GetMaxJitterTime() const;

    /**Get jitter time for received packets on remote.
       This is the calculated statistical variance of the interarrival
       time of received packets in milliseconds.
      */
    DWORD GetJitterTimeOnRemote() const;
  //@}

    virtual void SetCloseOnBYE(PBoolean v);

    /** Tell the rtp session to send out an intra frame request control packet.
        This is called when the media stream receives an OpalVideoUpdatePicture
        media command.
      */
    virtual void SendIntraFrameRequest(bool rfc2032, bool pictureLoss);

    void SetNextSentSequenceNumber(WORD num);

    virtual PString GetEncoding() const;
    virtual void SetEncoding(const PString & newEncoding);

    DWORD GetSyncSourceIn() const;

    class EncodingLock
    {
      public:
        EncodingLock(RTP_Session & _session);
        ~EncodingLock();

        //RTP_Encoding * operator->() const;
/* 		SIP_PYOBJECT __getattr__();
%MethodCode
		if ((sipRes = sipConvertFromNewType(sipCpp->m_encodingHandler, sipType_RTP_Encoding, NULL)) == NULL)
		{	
			sipIsErr = -1;
			PyErr_SetString(PyExc_RuntimeError, "Failed to convert to PyObject");
		}
%End */
	};

    void SetFailed(bool v);
    bool HasFailed() const;
    //void AddFilter(const PNotifier & filter);
	
	const RTCP_XR_Metrics & GetMetrics() const;
	
	virtual void SendBYE();
};

/**This class is for the IETF Real Time Protocol interface on UDP/IP.
 */
class RTP_UDP : RTP_Session
{

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    RTP_UDP(
      const RTP_Session::Params & options ///< Parameters to construct with session.
    );

    /// Destroy the RTP
    ~RTP_UDP();
  //@}

  /**@name Overrides from class RTP_Session */
  //@{
    /**Read a data frame from the RTP channel.
       Any control frames received are dispatched to callbacks and are not
       returned by this function. It will block until a data frame is
       available or an error occurs.
      */
    virtual PBoolean ReadData(RTP_DataFrame & frame);
    virtual PBoolean Internal_ReadData(RTP_DataFrame & frame);

    /** Write a data frame to the RTP channel.
      */
    virtual PBoolean WriteData(RTP_DataFrame & frame);
    virtual PBoolean Internal_WriteData(RTP_DataFrame & frame);

    /** Write data frame to the RTP channel outside the normal stream of media
      * Used for RFC2833 packets
      */
    virtual PBoolean WriteOOBData(RTP_DataFrame & frame, bool setTimeStamp = true);

    /**Write a control frame from the RTP channel.
      */
    virtual PBoolean WriteControl(RTP_ControlFrame & frame);

    /**Close down the RTP session.
      */
    virtual bool Close(
      PBoolean reading    ///<  Closing the read side of the session
    );

    /**Get the session description name.
      */
    virtual PString GetLocalHostName();
  //@}

    /**Change the QoS settings
      */
    virtual PBoolean ModifyQOS(RTP_QOS * rtpqos);

  /**@name New functions for class */
  //@{
    /**Open the UDP ports for the RTP session.
      */
    virtual PBoolean Open(
      PIPSocket::Address localAddress,  ///<  Local interface to bind to
      WORD portBase,                    ///<  Base of ports to search
      WORD portMax,                     ///<  end of ports to search (inclusive)
      BYTE ipTypeOfService,             ///<  Type of Service byte
      PNatMethod * natMethod = NULL,    ///<  NAT traversal method to use createing sockets
      RTP_QOS * rtpqos = NULL           ///<  QOS spec (or NULL if no QoS)
    );
  //@}

   /**Reopens an existing session in the given direction.
      */
    virtual void Reopen(PBoolean isReading);
  //@}

  /**@name Member variable access */
  //@{
    /**Get local address of session.
      */
    virtual PIPSocket::Address GetLocalAddress() const;

    /**Set local address of session.
      */
    virtual void SetLocalAddress(
      const PIPSocket::Address & addr
    );

    /**Get remote address of session.
      */
    PIPSocket::Address GetRemoteAddress() const;

    /**Get local data port of session.
      */
    virtual WORD GetLocalDataPort() const;

    /**Get local control port of session.
      */
    virtual WORD GetLocalControlPort() const;

    /**Get remote data port of session.
      */
    virtual WORD GetRemoteDataPort() const;

    /**Get remote control port of session.
      */
    virtual WORD GetRemoteControlPort() const;

    /**Get data UDP socket of session.
      */
    virtual PUDPSocket & GetDataSocket();

    /**Get control UDP socket of session.
      */
    virtual PUDPSocket & GetControlSocket();

    /**Set the remote address and port information for session.
      */
    virtual PBoolean SetRemoteSocketInfo(
      PIPSocket::Address address,   ///<  Addre ss of remote
      WORD port,                    ///<  Port on remote
      PBoolean isDataPort               ///<  Flag for data or control channel
    );

    /**Apply QOS - requires address to connect the socket on Windows platforms
     */
    virtual void ApplyQOS(
      const PIPSocket::Address & addr
    );
  //@}

    virtual int GetDataSocketHandle() const;

    virtual int GetControlSocketHandle() const;

    virtual int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timer);
    virtual int Internal_WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timer);

    virtual SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    virtual SendReceiveStatus Internal_ReadDataPDU(RTP_DataFrame & frame);

    virtual SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame);
    virtual SendReceiveStatus Internal_OnReadTimeout(RTP_DataFrame & frame);

    virtual SendReceiveStatus ReadControlPDU();
    virtual SendReceiveStatus ReadDataOrControlPDU(
      BYTE * framePtr,
      PINDEX frameSize,
      PBoolean fromDataChannel
    );

    virtual bool WriteDataPDU(RTP_DataFrame & frame);
    virtual bool WriteDataOrControlPDU(
      const BYTE * framePtr,
      PINDEX frameSize,
      bool toDataChannel
    );

    virtual void SetEncoding(const PString & newEncoding);
};

/////////////////////////////////////////////////////////////////////////////

class RTP_Encoding
{
  public:
    RTP_Encoding();
    virtual ~RTP_Encoding();
    virtual void ApplyStringOptions(const PStringToString & /*stringOptions*/);
    virtual void OnStart(RTP_Session & _rtpSession);
    virtual void OnFinish();
    virtual RTP_Session::SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual PBoolean WriteData(RTP_DataFrame & frame, bool oob);
    virtual PBoolean WriteDataPDU(RTP_DataFrame & frame);
    virtual void OnWriteDataIdle();
    virtual void SetWriteDataIdleTimer(PTimer &);
    virtual RTP_Session::SendReceiveStatus OnSendControl(RTP_ControlFrame & frame, PINDEX & len);
    virtual RTP_Session::SendReceiveStatus ReadDataPDU(RTP_DataFrame & frame);
    virtual RTP_Session::SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual RTP_Session::SendReceiveStatus OnReadTimeout(RTP_DataFrame & frame);
    virtual PBoolean ReadData(RTP_DataFrame & frame);
    virtual int WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval &);

    PMutex      mutex;
    unsigned    refCount;
};



/////////////////////////////////////////////////////////////////////////////

class SecureRTP_UDP : RTP_UDP
{

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    SecureRTP_UDP(
      const RTP_Session::Params & options ///< Parameters to construct with session.
    );

    /// Destroy the RTP
    ~SecureRTP_UDP();

    virtual void SetSecurityMode(OpalSecurityMode * srtpParms);  
    virtual OpalSecurityMode * GetSecurityParms() const;
};


/////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
// RTCP-XR - VoIP Metrics Report Block

/**This is a class used to calculate voice quality metrics acording to the 
   E-Model, proposed by ITU-T G.107 recomendation.	
  */
class RTCP_XR_Metrics
{  
%TypeHeaderCode
#include <ptlib.h>
#include <rtp/metrics.h>
%End
  public:
     RTCP_XR_Metrics();
    ~RTCP_XR_Metrics();
    
    enum PacketEvent {
      PACKET_RECEIVED,
      PACKET_DISCARDED,
      PACKET_LOST
    };  
        
    enum PeriodType {
      GAP,     /* a period of low packet losses and/or discards */
      BURST,   /* a period of a high proportion of packet losses and/or discards */     
    };
    
    /* A period of time, which can be a burst or a gap */
    struct TimePeriod {
      RTCP_XR_Metrics::PeriodType      type;
      PTimeInterval   duration; 
    };
    
    /**A period of time, used to calculate an average Id, needed to compute the R factor.
       Id factor represents the impairments caused by delay.
      */ 
    struct IdPeriod {
      PTimeInterval duration;
      float         Id;
    };
    
    /**A period of time, used to calculate an average Ie, needed to compute the R factor.
       Ie factor represents the impairments caused by low bit-rate codecs.
      */
    struct IePeriod {
      RTCP_XR_Metrics::PeriodType    type;
      PTimeInterval duration;
      float         Ieff;
    };
    
    enum QualityType {
      LQ,  /* Listening Quality, not include the effects of delay */
      CQ   /* Conversational Quality, include the effects of delay */
    };
       
    std::list<RTCP_XR_Metrics::TimePeriod> timePeriods;
    std::list<RTCP_XR_Metrics::IePeriod> iePeriods;
    std::list<RTCP_XR_Metrics::IdPeriod> idPeriods;
    
    /**Set the payload information.
       This must be called to obtain bitrate and E-Model parameters.
      */
    void SetPayloadInfo(
      RTP_DataFrame frame   ///<  Frame from which we get the payload info
    );
    
    /**Set the jitter buffer delay.
       This must be called to obtain the Id parameter.
      */
    void SetJitterDelay(
      DWORD delay   ///<  The jitter buffer delay in milliseconds
    );
    
    /**Called when a packet is received.
      */
    void OnPacketReceived();
    
    /**Called when a packet is discarded.
      */
    void OnPacketDiscarded();
    
    /**Called when a packet is lost.
      */
    void OnPacketLost();
     
    /**Called when several packets are lost.
      */
    void OnPacketLost(
      DWORD dropped   ///< Number of lost packets.
    );
    
    /**Called when a Sender Report is receveid.
      */
/*     void OnRxSenderReport(
      PUInt32b lsr,   ///< "Last SR" field of the received SR.
      PUInt32b dlsr   ///< "Delay since the last SR" field of the received SR.
    );
     */
    /**Get the fraction of RTP packets lost since the beginning of the reception.
      */
    BYTE GetLossRate();
    
    /**Get the fraction of RTP packets discarded since the beginning of the reception.
      */
    BYTE GetDiscardRate();
    
    /**Get the fraction of RTP packets lost/discarded within burst periods since
       the beginning of the reception.
      */
    BYTE GetBurstDensity();
    
    /**Get the fraction of RTP packets lost/discarded within gap periods since
       the beginning of the reception.
      */
    BYTE GetGapDensity();
    
    /**Get the mean duration, in milliseconds, of burst periods since the
       beginning of the reception.
     */
    WORD GetBurstDuration();
    
    /**Get the mean duration, in milliseconds, of gap periods since the beginning
       of the reception.
      */
    WORD GetGapDuration();
    
    /**Get the most recently calculated round trip time between RTP interfaces,
       expressed in millisecond.
      */
    WORD GetRoundTripDelay ();
    
    /**Get the most recently estimated end system delay, expressed in millisecond.
      */
    WORD GetEndSystemDelay();
    
    /**Get the R factor for the current RTP session, expressed in the range of 0 to 100.
      */
    BYTE RFactor();
    
    /**Get the estimated MOS score for listening quality of the current RTP session,
       expressed in the range of 10 to 50.
       This metric not includes the effects of delay.
      */
    BYTE MOS_LQ();
    
    /**Get the estimated MOS score for conversational quality of the current RTP session,
       expressed in the range of 10 to 50.
       This metric includes the effects of delay.
      */
    BYTE MOS_CQ();
};
