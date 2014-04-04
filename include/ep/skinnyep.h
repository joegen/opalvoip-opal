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
 *
 * $Revision$
 * $Author$
 * $Date$
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
      unsigned deviceType = DefaultDeviceType  ///< Device type code
    );

    /** Unregister from server.
      */
    bool Unregister(
      const PString & name         ///< Name of client "psuedo device" to unregister
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
        PhoneDevice(OpalSkinnyEndPoint & ep, const PString & name, unsigned deviceType);

        virtual void PrintOn(ostream & strm) const;

        bool Start(const PString & server);
        void Stop();
        void Close();

        bool SendSkinnyMsg(const SkinnyMsg & msg);

        const PString & GetName() const { return m_name; }
        const PString & GetStatus() const { return m_status; }
        OpalTransportAddress GetRemoteAddress() const { return m_transport.GetRemoteAddress(); }

      protected:
        void HandleTransport();
        bool SendRegisterMsg();

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

      friend class OpalSkinnyEndPoint;
      friend class OpalSkinnyConnection;
    };


#pragma pack(1)
    class SkinnyMsg : public PObject
    {
        PCLASSINFO(SkinnyMsg, PObject);
      protected:
        SkinnyMsg(uint32_t id, PINDEX len);
        void Construct(const PBYTEArray & pdu);

      public:
        uint32_t GetID() const { return m_messageId; }

        const BYTE * GetPacketPtr() const { return (const BYTE *)&m_length; }
        PINDEX       GetPacketLen() const { return m_length + sizeof(m_length) + sizeof(m_headerVersion); }

      protected:
        PUInt32l m_length;
        PUInt32l m_headerVersion;
        PUInt32l m_messageId;
    };

    // Note: all derived classes MUST NOT have composite members, e.g. PString
#define OPAL_SKINNY_MSG(cls, id, vars) \
    class cls : public SkinnyMsg \
    { \
        PCLASSINFO(cls, SkinnyMsg); \
      public: \
        enum { ID = id }; \
        cls() : SkinnyMsg(ID, sizeof(*this)) { } \
        cls(const PBYTEArray & pdu) : SkinnyMsg(ID, sizeof(*this)) { Construct(pdu); } \
        vars \
    }; \
    virtual bool OnReceiveMsg(PhoneDevice & client, const cls & msg)

    OPAL_SKINNY_MSG(KeepAliveMsg, 0x0000,
      PUInt32l m_unknown;
    );

    OPAL_SKINNY_MSG(KeepAliveAckMsg, 0x0100,
    );

    OPAL_SKINNY_MSG(RegisterMsg, 0x0001,
      enum { MaxNameSize = 15 };
      char     m_deviceName[MaxNameSize+1];
      PUInt32l m_userId;
      PUInt32l m_instance;
      in_addr  m_ip;
      PUInt32l m_deviceType;
      PUInt32l m_maxStreams;
      BYTE     m_unknown[16];
      char     m_macAddress[12];

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " device=" << m_deviceName << " type=" << m_deviceType << " streams=" << m_maxStreams << " ip=" << PIPAddress(m_ip); }
    );

    OPAL_SKINNY_MSG(RegisterAckMsg, 0x0081,
      PUInt32l m_keepAlive;
      char     m_dateFormat[6];
      BYTE     m_unknown1[2];
      PUInt32l m_secondaryKeepAlive;
      BYTE     m_unknown2[4];

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " keepAlive=" << m_keepAlive; }
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

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << ' ' << m_port; }
    );

    OPAL_SKINNY_MSG(CapabilityRequestMsg, 0x009B,
    );

    OPAL_SKINNY_MSG(CapabilityResponseMsg, 0x0010,
      PUInt32l m_count;
      struct Info
      {
        PUInt32l m_codec;
        PUInt16l m_maxFramesPerPacket;
        char     m_unknown[10];
      } m_capability[32];

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << ' ' << m_count << " codecs"; }

      void SetCount(PINDEX count)
      {
        m_count = count;
        m_length = m_length - (PARRAYSIZE(m_capability) - count) * sizeof(Info);
      }
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
      virtual void PrintOn(ostream & strm) const { strm << GetClass() << ' ' << GetState() << " line=" << m_lineInstance << " call=" << m_callIdentifier; }
    );

    P_DECLARE_STREAMABLE_ENUM(CallType,
      eTypeUnknownCall,
      eTypeInboundCall,
      eTypeOutboundCall,
      eTypeForwardCall
    );
    OPAL_SKINNY_MSG(CallInfoMsg, 0x008f,
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

      __inline CallType GetType() const { return (CallType)(uint32_t)m_callType; }
      virtual void PrintOn(ostream & strm) const { strm << GetClass() << ' ' << GetType() << " line=" << m_lineInstance << " call=" << m_callIdentifier; }
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
      virtual void PrintOn(ostream & strm) const { strm << GetClass() << ' ' << GetType() << " mode=" << m_ringMode << " line=" << m_lineInstance << " call=" << m_callIdentifier; }
    );

    OPAL_SKINNY_MSG(OffHookMsg, 0x0006,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " line=" << m_lineInstance << " call=" << m_callIdentifier; }
    );

    OPAL_SKINNY_MSG(OnHookMsg, 0x0007,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " line=" << m_lineInstance << " call=" << m_callIdentifier; }
    );

    enum Tones
    {
      eToneSilence     = 0x00,
      eToneDial        = 0x21,
      eToneBusy        = 0x23,
      eToneAlert       = 0x24,
      eToneReorder     = 0x25,
      eToneCallWaiting = 0x2d,
      eToneNoTone      = 0x7f
    };
    OPAL_SKINNY_MSG(StartToneMsg, 0x0082,
      PUInt32l m_tone;
      BYTE     m_unknown[4];
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      __inline Tones GetType() const { return (Tones)(uint32_t)m_tone; }
      virtual void PrintOn(ostream & strm) const { strm << GetClass() << ' ' << GetType() << " line=" << m_lineInstance << " call=" << m_callIdentifier; }
    );

    OPAL_SKINNY_MSG(StopToneMsg, 0x0083,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " line=" << m_lineInstance << " call=" << m_callIdentifier; }
    );

    OPAL_SKINNY_MSG(KeyPadButtonMsg, 0x0003,
      char     m_button;
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " \'" << m_button<< "' line=" << m_lineInstance << " call=" << m_callIdentifier; }
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
      virtual void PrintOn(ostream & strm) const { strm << GetClass() << ' ' << GetEvent() << " line=" << m_lineInstance << " call=" << m_callIdentifier; }
    );

    OPAL_SKINNY_MSG(OpenReceiveChannelMsg, 0x0105,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_msPerPacket;
      PUInt32l m_payloadCapability;
      PUInt32l m_echoCancelType;
      PUInt32l m_g723Bitrate;
      BYTE     m_unknown[68];

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " call=" << m_callIdentifier; }
    );

    OPAL_SKINNY_MSG(OpenReceiveChannelAckMsg, 0x0022,
      PUInt32l m_status;
      in_addr  m_ip;
      PUInt16l m_port;
      BYTE     m_padding[2];
      PUInt32l m_passThruPartyId;
      virtual void PrintOn(ostream & strm) const { strm << GetClass() << ' ' << PIPAddress(m_ip) << ':' << m_port; }
    );

    OPAL_SKINNY_MSG(CloseReceiveChannelMsg, 0x0106,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_conferenceId2;

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " call=" << m_callIdentifier; }
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

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " call=" << m_callIdentifier << ' ' << PIPAddress(m_ip) << ':' << m_port; }
    );

    OPAL_SKINNY_MSG(StopMediaTransmissionMsg, 0x008b,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_conferenceId2;

      virtual void PrintOn(ostream & strm) const { strm << GetClass() << " call=" << m_callIdentifier; }
    );
#pragma pack()

    /** Send a message to server.
      */
  //@}

  protected:
    PSafePtr<OpalSkinnyConnection> GetSkinnyConnection(const PhoneDevice & client, uint32_t callIdentifier, PSafetyMode mode = PSafeReadWrite);
    template <class MSG> bool DelegateMsg(const PhoneDevice & client, const MSG & msg);

    typedef PDictionary<PString, PhoneDevice> PhoneDeviceDict;
    PhoneDeviceDict m_phoneDevices;
    PMutex          m_phoneDevicesMutex;
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

    /** Get the remote transport address
      */
    virtual OpalTransportAddress GetRemoteAddress() const;
    //@}

  /**@name Protocol handling routines */
  //@{
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CallStateMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CallInfoMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::SetRingerMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::OpenReceiveChannelMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CloseReceiveChannelMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::StartMediaTransmissionMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::StopMediaTransmissionMsg & msg);
    //@}

  protected:
    OpalMediaSession * SetUpMediaSession(uint32_t payloadCapability, bool rx);
    OpalMediaType GetMediaTypeFromId(uint32_t id);
    void SetFromIdMediaType(const OpalMediaType & mediaType, uint32_t id);
    void DelayCloseMediaStream(OpalMediaStreamPtr mediaStream);

    OpalSkinnyEndPoint & m_endpoint;
    OpalSkinnyEndPoint::PhoneDevice & m_client;

    uint32_t m_lineInstance;
    uint32_t m_callIdentifier;
    PString  m_alertingType;
    bool     m_needSoftKeyEndcall;

    uint32_t m_audioId;
    uint32_t m_videoId;

    OpalMediaFormatList m_remoteMediaFormats;
};


template <class MSG> bool OpalSkinnyEndPoint::DelegateMsg(const PhoneDevice & client, const MSG & msg)
{
  PSafePtr<OpalSkinnyConnection> connection = GetSkinnyConnection(client, msg.m_callIdentifier);
  PTRACE_CONTEXT_ID_PUSH_THREAD(connection);
  return connection == NULL || connection->OnReceiveMsg(msg);
}


#endif // OPAL_SKINNY

#endif // OPAL_SKINNY_H
