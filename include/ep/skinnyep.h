/*
 * skinnyep.h
 *
 * Cisco SCCP (Skinny Client Control Protocol) support.
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2014 Vox Lucida Pty. Ltd.
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
 * Contributor(s): Robert Jongbloed (robertj@voxlucida.com.au).
 */

#ifndef OPAL_SKINNY_H
#define OPAL_SKINNY_H

#include <opal_config.h>

#if OPAL_SKINNY

#include <rtp/rtpep.h>
#include <rtp/rtpconn.h>


class OpalSkinnyConnection;


/**Endpoint for interfacing Cisco SCCP (Skinny Client Control Protocol).
 */
class OpalSkinnyEndPoint : public OpalRTPEndPoint
{
    PCLASSINFO(OpalSkinnyEndPoint, OpalRTPEndPoint);
  public:
    /**@name Construction */
    //@{
    /**Create a new endpoint.
     */
    OpalSkinnyEndPoint(
      OpalManager & manager,
      const char *prefix = "sccp"
      );

    /**Destroy endpoint.
     */
    virtual ~OpalSkinnyEndPoint();
    //@}

    /**@name Overrides from OpalEndPoint */
    //@{
    /** Shut down the endpoint, this is called by the OpalManager just before
        destroying the object and can be handy to make sure some things are
        stopped before the vtable gets clobbered.
        */
    virtual void ShutDown();

    /** Get the default transports for the endpoint type.
        Overrides the default behaviour to return udp and tcp.
        */
    virtual PString GetDefaultTransport() const;

    /** Get the default signal port for this endpoint.
      */
    virtual WORD GetDefaultSignalPort() const;

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.
       */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /** Handle new incoming connection from listener.
      */
    virtual void NewIncomingConnection(
      OpalListener & listener,            ///<  Listner that created transport
      const OpalTransportPtr & transport  ///<  Transport connection came in on
      );

    /** Set up a connection to a remote party.
        This is called from the OpalManager::MakeConnection() function once
        it has determined that this is the endpoint for the protocol.

        The general form for this party parameter is:

        [proto:][alias@][transport$]address[:port]

        where the various fields will have meanings specific to the endpoint
        type. For example, with H.323 it could be "h323:Fred@site.com" which
        indicates a user Fred at gatekeeper size.com. Whereas for the PSTN
        endpoint it could be "pstn:5551234" which is to call 5551234 on the
        first available PSTN line.

        The proto field is optional when passed to a specific endpoint. If it
        is present, however, it must agree with the endpoints protocol name or
        false is returned.

        This function usually returns almost immediately with the connection
        continuing to occur in a new background thread.

        If false is returned then the connection could not be established. For
        example if a PSTN endpoint is used and the assiciated line is engaged
        then it may return immediately. Returning a non-NULL value does not
        mean that the connection will succeed, only that an attempt is being
        made.
        */
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,                         ///<  Owner of connection
      const PString & party,                   ///<  Remote party to call
      void * userData,                         ///<  Arbitrary data to pass to connection
      unsigned int options,                    ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions  ///<  complex string options
      );

    /** Execute garbage collection for endpoint.
        Returns true if all garbage has been collected.
        */
    virtual PBoolean GarbageCollection();
    //@}


    class PhoneDevice;

    /**@name Customisation call backs */
    //@{
    /** Create a connection for the skinny endpoint.
      */
    virtual OpalSkinnyConnection * CreateConnection(
      OpalCall & call,            ///< Owner of connection
      PhoneDevice & client,            ///< Registered client for call
      unsigned callIdentifier,    ///< Identifier for call
      const PString & dialNumber, ///< Number to dial out, empty if incoming
      void * userData,            ///< Arbitrary data to pass to connection
      unsigned int options,
      OpalConnection::StringOptions * stringOptions = NULL
      );
    //@}

    /**@name Protocol handling routines */
    //@{
    enum { DefaultDeviceType = 30016 }; // Cisco IP Communicator

    /** Register with skinny server.
      */
    bool Register(
      const PString & server,      ///< Server to register with
      const PString & name,        ///< Name of cient "psuedo device" to register
      unsigned deviceType = DefaultDeviceType,  ///< Device type code
      const PString & localInterface = PString::Empty()  ///< Local binding interface
    );

    /** Unregister from server.
      */
    bool Unregister(
      const PString & name         ///< Name of client "psuedo device" to unregister
    );

    /// Create a new PhoneDevice object
    PhoneDevice * CreatePhoneDevice(
      const PString & name,             ///< Name of cient "psuedo device" to register
      unsigned deviceType,              ///< Device type code
      const PIPAddressAndPort & binding ///< Local binding interface/port
    );

    PhoneDevice * GetPhoneDevice(
      const PString & name         ///< Name of client "psuedo device" to unregister
    ) const { return m_phoneDevices.GetAt(name); }

    PArray<PString> GetPhoneDeviceNames() const { return m_phoneDevices.GetKeys(); }
    //@}


    class SkinnyMsg;

    class PhoneDevice : public PObject
    {
      PCLASSINFO(PhoneDevice, PObject);
      public:
        PhoneDevice(OpalSkinnyEndPoint & ep, const PString & name, unsigned deviceType, const PIPAddressAndPort & binding);
        ~PhoneDevice() { Close(); }

        virtual void PrintOn(ostream & strm) const;

        bool Start(const PString & server);
        bool Stop();
        void Close();

        bool SendSkinnyMsg(const SkinnyMsg & msg);

        const PString & GetName() const { return m_name; }
        unsigned GetDeviceType() const { return m_deviceType; }
        const PString & GetStatus() const { return m_status; }
        OpalTransportAddress GetRemoteAddress() const { return m_transport.GetRemoteAddress(); }

      protected:
        void HandleTransport();
        bool SendRegisterMsg();
        PDECLARE_NOTIFIER(PTimer, OpalSkinnyEndPoint::PhoneDevice, OnKeepAlive);

        template <class MSG> bool OnReceiveMsg(const MSG & msg)
        {
          PTRACE(3, "Skinny", "Received " << msg);
          return m_endpoint.OnReceiveMsg(*this, msg);
        }

        OpalSkinnyEndPoint & m_endpoint;
        PString              m_name;
        unsigned             m_deviceType;
        OpalTransportTCP     m_transport;
        PTimeInterval        m_delay;
        PString              m_status;
        PSyncPoint           m_exit;
        PTimer               m_keepAliveTimer;

        // Currently only support one call at a time
        PSafePtr<OpalSkinnyConnection> m_activeConnection;

      friend class OpalSkinnyEndPoint;
      friend class OpalSkinnyConnection;
    };


#pragma pack(1)
    class SkinnyMsg : public PObject
    {
        PCLASSINFO(SkinnyMsg, PObject);
      protected:
        SkinnyMsg(uint32_t id, PINDEX len, PINDEX extraSpace);
        void Construct(const PBYTEArray & pdu);

      public:
        uint32_t GetID() const { return m_messageId; }

        const BYTE * GetPacketPtr() const { return (const BYTE *)&m_length; }
        PINDEX       GetPacketLen() const { return m_length + sizeof(m_length) + sizeof(m_headerVersion); }

      protected:
        PINDEX   m_extraSpace;

        // Data after here is mapped to "over the wire" PDU, no polymorhic classes
        PUInt32l m_length;
        PUInt32l m_headerVersion;
        PUInt32l m_messageId;
    };

    // Note: all derived classes MUST NOT have composite members, e.g. PString
#define OPAL_SKINNY_MSG2(cls, base, id, extraSpace, vars) \
    class cls : public base \
    { \
        PCLASSINFO(cls, base); \
      public: \
        enum { ID = id }; \
        cls() : base(ID, sizeof(*this), extraSpace) { } \
        cls(const PBYTEArray & pdu) : base(ID, sizeof(*this), extraSpace) { Construct(pdu); } \
        vars \
    }; \
    virtual bool OnReceiveMsg(PhoneDevice & client, const cls & msg)

#define OPAL_SKINNY_MSG(cls, id, vars) OPAL_SKINNY_MSG2(cls, SkinnyMsg, id, 0, vars)

    OPAL_SKINNY_MSG(KeepAliveMsg, 0x0000,
      PUInt32l m_unknown;
    );

    OPAL_SKINNY_MSG(KeepAliveAckMsg, 0x0100,
    );

    struct Proto {
      uint8_t  m_version;
      uint8_t  m_flags;
      BYTE     m_unknown1;
      uint8_t  m_features; // 0x01=Dynamic Messages, 0x80=Abbreviated dialing

      friend ostream & operator<<(ostream & strm, const Proto & protocol);
    };

    OPAL_SKINNY_MSG(RegisterMsg, 0x0001,
      enum { MaxNameSize = 15 };
      char     m_deviceName[MaxNameSize+1];
      PUInt32l m_userId;
      PUInt32l m_instance;
      in_addr  m_ip;
      PUInt32l m_deviceType;
      PUInt32l m_maxStreams;
      PUInt32l m_activeStreams;
      Proto    m_protocol;
      PUInt32l m_socketType;    // 0=ASCII, 1=HEX
      BYTE     m_unknown2[4];
      char     m_macAddress[12];

      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(RegisterAckMsg, 0x0081,
      PUInt32l m_keepAlive;
      char     m_dateFormat[6];
      BYTE     m_unknown1[2];
      PUInt32l m_secondaryKeepAlive;
      Proto    m_protocol;

      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(RegisterRejectMsg, 0x009d,
      char m_errorText[32];
    );

    OPAL_SKINNY_MSG(UnregisterMsg, 0x0027,
    );

    OPAL_SKINNY_MSG(UnregisterAckMsg, 0x0118,
      PUInt32l m_status;
    );

    OPAL_SKINNY_MSG(PortMsg, 0x0002,
      PUInt16l m_port;

    virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(CapabilityRequestMsg, 0x009B,
    );

    OPAL_SKINNY_MSG(CapabilityResponseMsg, 0x0010,
      PUInt32l m_count;
      struct Info
      {
        PUInt32l m_codec;
        PUInt32l m_maxFramesPerPacket;
        PUInt32l m_g7231BitRate; // 1=5.3 Kbps, 2=6.4 Kbps
        char     m_unknown[4];
      } m_capability[32];

      virtual void PrintOn(ostream & strm) const;
      void SetCount(PINDEX count);
    );

    P_DECLARE_STREAMABLE_ENUM(CallStates,
      eUnknownState,
      eStateOffHook,
      eStateOnHook,
      eStateRingOut,
      eStateRingIn,
      eStateConnected,
      eStateBusy,
      eStateCongestion,
      eStateHold,
      eStateCallWaiting,
      eStateCallTransfer,
      eStateCallPark,
      eStateProceed,
      eStateCallRemoteMultiline,
      eStateInvalidNumber
    );
    OPAL_SKINNY_MSG(CallStateMsg, 0x0111,
      PUInt32l m_state;
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;
      BYTE     m_unknown[12];

      __inline CallStates GetState() const { return (CallStates)(uint32_t)m_state; }
      virtual void PrintOn(ostream & strm) const;
    );

    P_DECLARE_STREAMABLE_ENUM(CallType,
      eTypeUnknownCall,
      eTypeInboundCall,
      eTypeOutboundCall,
      eTypeForwardCall
    );

    class CallInfoCommon : public SkinnyMsg
    {
    protected:
      CallInfoCommon(uint32_t id, PINDEX len, PINDEX extraSpace) : SkinnyMsg(id, len, extraSpace) { }
    public:
      virtual const PUInt32l & GetLineInstance() const = 0;
      virtual const PUInt32l & GetCallIdentifier() const = 0;
      virtual CallType GetType() const = 0;
      virtual const char * GetCalledPartyName() const = 0;
      virtual const char * GetCalledPartyNumber() const = 0;
      virtual const char * GetCallingPartyName() const = 0;
      virtual const char * GetCallingPartyNumber() const = 0;
      virtual const char * GetRedirectingPartyNumber() const = 0;

      virtual void PrintOn(ostream & strm) const;
    };

    OPAL_SKINNY_MSG2(CallInfoMsg,  CallInfoCommon, 0x008f, 0,
      char     m_callingPartyName[40];
      char     m_callingPartyNumber[24];
      char     m_calledPartyName[40];
      char     m_calledPartyNumber[24];
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;
      PUInt32l m_callType;
      char     m_originalCalledPartyName[40];
      char     m_originalCalledPartyNumber[24];
      char     m_lastRedirectingPartyName[40];
      char     m_lastRedirectingPartyNumber[24];
      PUInt32l m_originalCalledPartyRedirectReason;
      PUInt32l m_lastRedirectingReason;
      char     m_callingPartyVoiceMailbox[24];
      char     m_calledPartyVoiceMailbox[24];
      char     m_originalCalledPartyVoiceMailbox[24];
      char     m_lastRedirectingVoiceMailbox[24];
      PUInt32l m_callInstance;
      PUInt32l m_callSecurityStatus;
      PUInt32l m_partyPIRestrictionBits;

      virtual const PUInt32l & GetLineInstance() const { return m_lineInstance; }
      virtual const PUInt32l & GetCallIdentifier() const { return m_callIdentifier; }
      virtual CallType GetType() const { return (CallType)(uint32_t)m_callType; }
      virtual const char * GetCalledPartyName() const { return m_calledPartyName; }
      virtual const char * GetCalledPartyNumber() const { return m_calledPartyNumber; }
      virtual const char * GetCallingPartyName() const { return m_callingPartyName; }
      virtual const char * GetCallingPartyNumber() const { return m_callingPartyNumber; }
      virtual const char * GetRedirectingPartyNumber() const { return m_lastRedirectingPartyNumber; }
    );

    enum { CallInfo5MsgStringSpace = 200 };
    OPAL_SKINNY_MSG2(CallInfo5Msg, CallInfoCommon, 0x014a, CallInfo5MsgStringSpace,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;
      PUInt32l m_callType;
      PUInt32l m_originalCalledPartyRedirectReason;
      PUInt32l m_lastRedirectingReason;
      PUInt32l m_callInstance;
      PUInt32l m_callSecurityStatus;
      PUInt32l m_partyPIRestrictionBits;
      char     m_strings[CallInfo5MsgStringSpace+11]; // Allow space for strings

      virtual const PUInt32l & GetLineInstance() const { return m_lineInstance; }
      virtual const PUInt32l & GetCallIdentifier() const { return m_callIdentifier; }
      virtual CallType GetType() const { return (CallType)(uint32_t)m_callType; }
      virtual const char * GetCalledPartyName() const { return GetStringByIndex(9); }
      virtual const char * GetCalledPartyNumber() const { return GetStringByIndex(1); }
      virtual const char * GetCallingPartyName() const { return GetStringByIndex(8); }
      virtual const char * GetCallingPartyNumber() const { return GetStringByIndex(0); }
      virtual const char * GetRedirectingPartyNumber() const { return GetStringByIndex(3); }

      protected:
        const char * GetStringByIndex(PINDEX idx) const;
    );

    enum RingType
    {
      eRingOff = 1,
      eRingInside,
      eRingOutside,
      eRingFeature
    };
    OPAL_SKINNY_MSG(SetRingerMsg, 0x0085,
      PUInt32l m_ringType;
      PUInt32l m_ringMode;
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      __inline RingType GetType() const { return (RingType)(uint32_t)m_ringType; }
      __inline bool IsForever() const { return m_ringMode == 1; }
      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(OffHookMsg, 0x0006,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(OnHookMsg, 0x0007,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      virtual void PrintOn(ostream & strm) const;
    );

    enum Tones
    {
      eToneSilence     = 0,
      eToneDial        = 33,
      eToneBusy        = 35,
      eToneAlert       = 36,
      eToneReorder     = 37,
      eToneCallWaiting = 45,
      eToneNoTone      = 0x7f
    };
    OPAL_SKINNY_MSG(StartToneMsg, 0x0082,
      PUInt32l m_tone;
      BYTE     m_unknown[4];
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      __inline Tones GetType() const { return (Tones)(uint32_t)m_tone; }
      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(StopToneMsg, 0x0083,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(KeyPadButtonMsg, 0x0003,
      BYTE     m_dtmf;
      BYTE     m_padding[3];
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      virtual void PrintOn(ostream & strm) const;
    );

    P_DECLARE_STREAMABLE_ENUM(SoftKeyEvents,
      eSoftUnknown,
      eSoftKeyRedial,
      eSoftKeyNewCall,
      eSoftKeyHold,
      eSoftKeyTransfer,
      eSoftKeyCfwdall,
      eSoftKeyCfwdBusy,
      eSoftKeyCfwdNoAnswer,
      eSoftKeyBackspace,
      eSoftKeyEndcall,
      eSoftKeyResume,
      eSoftKeyAnswer,
      eSoftKeyInfo,
      eSoftKeyConf,
      eSoftKeyPark,
      eSoftKeyJoin,
      eSoftKeyMeetMe,
      eSoftKeyCallPickup,
      eSoftKeyGrpCallPickup,
      eSoftKeyDND,
      eSoftKeyDivert
    );
    OPAL_SKINNY_MSG(SoftKeyEventMsg, 0x0026,
      PUInt32l m_event;
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      __inline SoftKeyEvents GetEvent() const { return (SoftKeyEvents)(uint32_t)m_event; }
      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(OpenReceiveChannelMsg, 0x0105,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_msPerPacket;
      PUInt32l m_payloadCapability;
      PUInt32l m_echoCancelType;
      PUInt32l m_g723Bitrate;
      BYTE     m_unknown[68];

      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(OpenReceiveChannelAckMsg, 0x0022,
      PUInt32l m_status;
      in_addr  m_ip;
      PUInt16l m_port;
      BYTE     m_padding[2];
      PUInt32l m_passThruPartyId;
      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(CloseReceiveChannelMsg, 0x0106,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_conferenceId2;

      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(StartMediaTransmissionMsg, 0x008a,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      in_addr  m_ip;
      PUInt16l m_port;
      BYTE     m_padding[2];
      PUInt32l m_msPerPacket;
      PUInt32l m_payloadCapability;
      PUInt32l m_precedence;
      PUInt32l m_silenceSuppression;
      PUInt32l m_maxFramesPerPacket;
      PUInt32l m_g723Bitrate;
      BYTE     m_unknown[68];

      virtual void PrintOn(ostream & strm) const;
    );

    OPAL_SKINNY_MSG(StopMediaTransmissionMsg, 0x008b,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_conferenceId2;

      virtual void PrintOn(ostream & strm) const;
    );
#pragma pack()
  //@}

    const PFilePath & GetSimulatedFarAudioFile() const { return m_simulatedFarAudioFile; }
    void SetSimulatedFarAudioFile(const PString str) { m_simulatedFarAudioFile = str; }
    const PFilePath & GetSimulatedNearAudioFile() const { return m_simulatedNearAudioFile; }
    void SetSimulatedNearAudioFile(const PString str) { m_simulatedNearAudioFile = str; }

    const PDirectory & GetSimulatedAudioFolder() const { return m_simulatedAudioFolder; }
    void SetSimulatedAudioFolder(const PString str) { m_simulatedAudioFolder = str; }

    PFilePath GetEndpointSimulatedAudioFile(const PString& name, unsigned sessionId) const;

    bool IsSecondaryAudioAlwaysSimulated() const { return m_secondaryAudioAlwaysSimulated; }
    void SetSecondaryAudioAlwaysSimulated(bool v) { m_secondaryAudioAlwaysSimulated = v; }

    void SetServerInterfaces(const PString & addresses) { m_serverInterfaces = OpalTransportAddressArray(addresses); }

  protected:
    template <class MSG> bool DelegateMsg(const PhoneDevice & client, const MSG & msg);

    typedef PDictionary<PString, PhoneDevice> PhoneDeviceDict;
    PhoneDeviceDict           m_phoneDevices;
    PDECLARE_MUTEX(m_phoneDevicesMutex);
    OpalTransportAddressArray m_serverInterfaces;
    PFilePath                 m_simulatedFarAudioFile;
    PFilePath                 m_simulatedNearAudioFile;
    PDirectory                m_simulatedAudioFolder;
    bool                      m_secondaryAudioAlwaysSimulated;
};


/**Connection for interfacing Cisco SCCP (Skinny PhoneDevice Control Protocol).
  */
class OpalSkinnyConnection : public OpalRTPConnection
{
    PCLASSINFO(OpalSkinnyConnection, OpalRTPConnection);
  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    OpalSkinnyConnection(
      OpalCall & call,
      OpalSkinnyEndPoint & ep,
      OpalSkinnyEndPoint::PhoneDevice & client,
      unsigned callIdentifier,
      const PString & dialNumber,
      void * /*userData*/,
      unsigned options,
      OpalConnection::StringOptions * stringOptions
    );
  //@}

  /**@name Overrides from OpalConnection */
  //@{
    /** Get indication of connection being to a "network".
        This indicates the if the connection may be regarded as a "network"
        connection. The distinction is about if there is a concept of a "remote"
        party being connected to and is best described by example: sip, h323,
        iax and pstn are all "network" connections as they connect to something
        "remote". While pc, pots and ivr are not as the entity being connected
        to is intrinsically local.
    */
    virtual bool IsNetworkConnection() const { return true; }

    /** Start an outgoing connection.
        This function will initiate the connection to the remote entity, for
        example in H.323 it sends a SETUP, in SIP it sends an INVITE etc.
    */
    virtual PBoolean SetUpConnection();

    /** Clean up the termination of the connection.
        This function can do any internal cleaning up and waiting on background
        threads that may be using the connection object.

        Note that there is not a one to one relationship with the
        OnEstablishedConnection() function. This function may be called without
        that function being called. For example if SetUpConnection() was used
        but the call never completed.

        Classes that override this function should make sure they call the
        ancestor version for correct operation.

        An application will not typically call this function as it is used by
        the OpalManager during a release of the connection.
      */
    virtual void OnReleased();

    /**Get the data formats this connection is capable of operating.
       This provides a list of media data format names that a
       OpalMediaStream may be created in within this connection.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;

    /**Indicate to remote endpoint an alert is in progress.
       If this is an incoming connection and the AnswerCallResponse is in a
       AnswerCallDeferred or AnswerCallPending state, then this function is
       used to indicate to that endpoint that an alert is in progress. This is
       usually due to another connection which is in the call (the B party)
       has received an OnAlerting() indicating that its remoteendpoint is
       "ringing".
      */
    virtual PBoolean SetAlerting(
      const PString & calleeName,   ///<  Name of endpoint being alerted.
      PBoolean withMedia                ///<  Open media with alerting
    );

    /**Indicate to remote endpoint we are connected.
      */
    virtual PBoolean SetConnected();

    /** Indicate whether a particular media type can auto-start.
        This is typically used for things like video or fax to contol if on
        initial connection, that media type is opened straight away. Streams
        of that type may be opened later, during the call, by using the
        OpalCall::OpenSourceMediaStreams() function.
    */
    virtual OpalMediaType::AutoStartMode GetAutoStart(
      const OpalMediaType & mediaType  ///< media type to check
    ) const;

    /** Get alerting type information of an incoming call.
        The type of "distinctive ringing" for the call. The string is protocol
        dependent, so the caller would need to be aware of the type of call
        being made. Some protocols may ignore the field completely.

        For SIP this corresponds to the string contained in the "Alert-Info"
        header field of the INVITE. This is typically a URI for the ring file.

        For H.323 this must be a string representation of an integer from 0 to 7
        which will be contained in the Q.931 SIGNAL (0x34) Information Element.
    */
    virtual PString GetAlertingType() const;

    /**Call back for closed a media stream.

       The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnClosedMediaStream(
      const OpalMediaStream & stream     ///<  Media stream being closed
    );

    /** Get the remote transport address
      */
    virtual OpalTransportAddress GetRemoteAddress() const;
    //@}

  /**@name Protocol handling routines */
  //@{
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CallStateMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CallInfoMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CallInfo5Msg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::SetRingerMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::StartToneMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::OpenReceiveChannelMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CloseReceiveChannelMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::StartMediaTransmissionMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::StopMediaTransmissionMsg & msg);
    //@}

  protected:
    bool OnReceiveCallInfo(const OpalSkinnyEndPoint::CallInfoCommon & msg);

    struct MediaInfo
    {
      MediaInfo(const OpalSkinnyEndPoint::OpenReceiveChannelMsg & msg);
      MediaInfo(const OpalSkinnyEndPoint::CloseReceiveChannelMsg & msg);
      MediaInfo(const OpalSkinnyEndPoint::StartMediaTransmissionMsg & msg);
      MediaInfo(const OpalSkinnyEndPoint::StopMediaTransmissionMsg & msg);

      bool operator<(const MediaInfo & other) const;

      bool                 m_receiver;
      uint32_t             m_passThruPartyId;
      uint32_t             m_payloadCapability;
      OpalTransportAddress m_mediaAddress;
      mutable unsigned     m_sessionId;
    };
    void OpenMediaChannel(const MediaInfo & info);
    void OpenSimulatedMediaChannel(const MediaInfo & info, const OpalMediaFormat & mediaFormat);
    void DelayCloseMediaStream(OpalMediaStreamPtr mediaStream);

    OpalSkinnyEndPoint & m_endpoint;
    OpalSkinnyEndPoint::PhoneDevice & m_phoneDevice;

    uint32_t m_lineInstance;
    uint32_t m_callIdentifier;
    PString  m_alertingType;
    bool     m_needSoftKeyEndcall;
    bool     m_remoteHold;

    OpalMediaFormatList m_remoteMediaFormats;
    std::set<MediaInfo> m_passThruMedia;
    std::set<unsigned>  m_simulatedTransmitters;
};


template <class MSG> bool OpalSkinnyEndPoint::DelegateMsg(const PhoneDevice & phone, const MSG & msg)
{
  PSafePtr<OpalSkinnyConnection> connection = phone.m_activeConnection;
  PTRACE_CONTEXT_ID_PUSH_THREAD(connection);
  return connection == NULL || !connection.SetSafetyMode(PSafeReadWrite) || connection->OnReceiveMsg(msg);
}


#endif // OPAL_SKINNY

#endif // OPAL_SKINNY_H
