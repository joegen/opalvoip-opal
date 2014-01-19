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
        Default behaviour deletes the objects in the connectionsActive list.
    */
    virtual PBoolean GarbageCollection();
  //@}


  /**@name Customisation call backs */
  //@{
    /** Create a connection for the skinny endpoint.
        The default implementation is to create a OpalSkinnyConnection.
      */
    virtual OpalSkinnyConnection * CreateConnection(
      OpalCall & call,            ///< Owner of connection
      const PString & token,      ///< Token to identify call
      const PString & dialNumber, ///< Number to dial out, empty if incoming
      void * userData,            ///< Arbitrary data to pass to connection
      unsigned int options,
      OpalConnection::StringOptions * stringOptions = NULL
    );
  //@}

  /**@name Protocol handling routines */
  //@{
    /** Register with skinny server.
      */
    bool Register(
      const PString & server,   ///< Server to register with
      unsigned deviceType = 30016 ///< Device type code (Cisco IP Communicator)
    );

#pragma pack(1)
    class SkinnyMsg
    {
      protected:
        SkinnyMsg(uint32_t id, PINDEX len);
        SkinnyMsg(const PBYTEArray & pdu, PINDEX len);

      public:
        PINDEX GetLength() const { return m_length+8; }
        void SetLength(PINDEX len) { PAssert(len>= 8, PInvalidParameter); m_length = len-8; }
        uint32_t GetID() const { return m_messageId; }

      private:
        PUInt32l m_length;
        PUInt32l m_headerVersion;
        PUInt32l m_messageId;
    };

    #define OPAL_SKINNY_MSG(cls, id, vars) \
      class cls : public SkinnyMsg \
      { \
        public: \
          enum { ID = id }; \
          cls() : SkinnyMsg(ID, sizeof(*this)) { } \
          cls(const PBYTEArray & pdu) : SkinnyMsg(pdu, sizeof(*this)) { } \
          vars \
      }; \
      virtual bool OnReceiveMsg(const cls & msg)

    OPAL_SKINNY_MSG(KeepAliveMsg, 0x0000,
      PUInt32l m_unknown;
    );

    OPAL_SKINNY_MSG(KeepAliveAckMsg, 0x0100,
    );

    OPAL_SKINNY_MSG(RegisterMsg, 0x0001,
      char     m_deviceName[16];
      PUInt32l m_userId;
      PUInt32l m_instance;
      in_addr  m_ip;
      PUInt32l m_deviceType;
      PUInt32l m_maxStreams;
      BYTE     m_unknown[28];
    );

    OPAL_SKINNY_MSG(RegisterAckMsg, 0x0081,
      PUInt32l m_keepAlive;
      char     m_dateFormat[6];
      BYTE     m_unknown1[2];
      PUInt32l m_secondaryKeepAlive;
      BYTE     m_unknown2[4];
    );

    OPAL_SKINNY_MSG(RegisterRejectMsg, 0x009d,
      char m_errorText[33];
    );

    OPAL_SKINNY_MSG(UnregisterMsg, 0x0027,
    );

    OPAL_SKINNY_MSG(UnregisterAckMsg, 0x0118,
      PUInt32l m_status;
    );

    OPAL_SKINNY_MSG(PortMsg, 0x0002,
      PUInt16l m_port;
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
    );

    enum CallStates
    {
      eStateOffHook = 1,
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
    };
    OPAL_SKINNY_MSG(CallStateMsg, 0x0111,
      PUInt32l m_state;
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      __inline CallStates GetState() const { return (CallStates)(uint32_t)m_state; }
    );

    enum RingType
    {
      eRingOff = 1,
      eRongInside,
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
    );

    OPAL_SKINNY_MSG(OffHookMsg, 0x0006,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;
    );

    OPAL_SKINNY_MSG(OnHookMsg, 0x0007,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;
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
    );

    OPAL_SKINNY_MSG(StopToneMsg, 0x0083,
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;
    );

    OPAL_SKINNY_MSG(KeyPadButtonMsg, 0x0003,
      char     m_button;
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;
    );

    enum SoftKeyEvents
    {
      eSoftKeyRedial = 1,
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
      eSoftKeyDNS,
      eSoftKeyDivert
    };
    OPAL_SKINNY_MSG(SoftKeyEventMsg, 0x0026,
      PUInt32l m_event;
      PUInt32l m_lineInstance;
      PUInt32l m_callIdentifier;

      __inline SoftKeyEvents GetEvent() const { return (SoftKeyEvents)(uint32_t)m_event; }
    );

    OPAL_SKINNY_MSG(OpenReceiveChannelMsg, 0x0105,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_msPerPacket;
      PUInt32l m_payloadCapability;
      PUInt32l m_echoCancelType;
      PUInt32l m_g723Bitrate;
      BYTE     m_unknown[84];
    );

    OPAL_SKINNY_MSG(OpenReceiveChannelAckMsg, 0x0022,
      PUInt32l m_status;
      in_addr  m_ip;
      PUInt16l m_port;
      BYTE     m_padding[2];
      PUInt32l m_passThruPartyId;
    );

    OPAL_SKINNY_MSG(CloseReceiveChannelMsg, 0x0106,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_conferenceId2;
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
      BYTE     m_unknown[76];
    );

    OPAL_SKINNY_MSG(StopMediaTransmissionMsg, 0x008b,
      PUInt32l m_callIdentifier;
      PUInt32l m_passThruPartyId;
      PUInt32l m_conferenceId2;
    );
#pragma pack()

    /** Send a message to server.
      */
    bool SendSkinnyMsg(const SkinnyMsg & msg);
  //@}

  protected:
    void HandleServerTransport();
    PSafePtr<OpalSkinnyConnection> GetSkinnyConnection(uint32_t callIdentifier, PSafetyMode mode = PSafeReadWrite);

    template <class MSG> bool DelegateMsg(const MSG & msg)
    {
      PSafePtr<OpalSkinnyConnection> connection = GetSkinnyConnection(msg.m_callIdentifier);
      return connection == NULL || connection->OnReceiveMsg(msg);
    }


    OpalTransportTCP m_serverTransport;
};


/**Connection for interfacing Cisco SCCP (Skinny Client Control Protocol).
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
      const PString & token,
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

        The default behaviour does.
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

        The default behaviour calls the OpalEndPoint function of the same name.
      */
    virtual void OnReleased();

    /**Indicate to remote endpoint we are connected.
      */
    virtual PBoolean SetConnected();

    /** Get alerting type information of an incoming call.
        The type of "distinctive ringing" for the call. The string is protocol
        dependent, so the caller would need to be aware of the type of call
        being made. Some protocols may ignore the field completely.

        For SIP this corresponds to the string contained in the "Alert-Info"
        header field of the INVITE. This is typically a URI for the ring file.

        For H.323 this must be a string representation of an integer from 0 to 7
        which will be contained in the Q.931 SIGNAL (0x34) Information Element.

        Default behaviour returns an empty string.
    */
    virtual PString GetAlertingType() const;
    //@}

  /**@name Protocol handling routines */
  //@{
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::SetRingerMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CallStateMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::OpenReceiveChannelMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::CloseReceiveChannelMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::StartMediaTransmissionMsg & msg);
    virtual bool OnReceiveMsg(const OpalSkinnyEndPoint::StopMediaTransmissionMsg & msg);
    //@}

  protected:
    OpalMediaSession * SetUpMediaSession(uint32_t payloadCapability, bool rx);
    OpalMediaType GetMediaTypeFromId(uint32_t id);
    void SetFromIdMediaType(const OpalMediaType & mediaType, uint32_t id);

    OpalSkinnyEndPoint & m_endpoint;

    uint32_t m_lineInstance;
    uint32_t m_callIdentifier;
    PString  m_alertingType;

    uint32_t m_audioId;
    uint32_t m_videoId;
};

#endif // OPAL_SKINNY

#endif // OPAL_SKINNY_H
