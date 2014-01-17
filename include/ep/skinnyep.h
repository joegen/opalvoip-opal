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

    /** A call back function whenever a connection is broken.
        This function can do any internal cleaning up and waiting on background
        threads that may be using the connection object.

        Note that there is not a one to one relationship with the
        OnEstablishedConnection() function. This function may be called without
        that function being called. For example if MakeConnection() was used
        but the call never completed.

        Classes that override this function should make sure they call the
        ancestor version for correct operation.

        An application will not typically call this function as it is used by
        the OpalManager during a release of the connection.

        The default behaviour removes the connection from the internal database
        and calls the OpalManager function of the same name.
    */
    virtual void OnReleased(
      OpalConnection & connection   ///<  Connection that was established
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
      OpalCall & call,        ///<  Owner of connection
      const PString & number, ///<  Number ot dial, empty for incoming call
      void * userData,        ///<  Arbitrary data to pass to connection
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
      unsigned deviceType = 8 ///< Device type code (gateway virtual phone)
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

    class RegisterMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0001 };
        RegisterMsg(
          const char * deviceName,
          unsigned userId,
          unsigned instance,
          const PIPSocket::Address & ip,
          unsigned deviceType,
          unsigned maxStreams
        );

      protected:
        char     m_deviceName[16];
        PUInt32l m_userId;
        PUInt32l m_instance;
        in_addr  m_ip;
        PUInt32l m_deviceType;
        PUInt32l m_maxStreams;
        BYTE     m_unknown[28];
    };

    class RegisterAckMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0081 };
        RegisterAckMsg(const PBYTEArray & pdu);

        PTimeInterval GetKeepALive() const { return PTimeInterval(0, m_keepAlive); }

      protected:
        PUInt32l m_keepAlive;
        char     m_dateFormat[6];
        char     m_reserved1[2];
        PUInt32l m_secondaryKeepAlive;
        char     m_reserved2[4];
    };

    class RegisterRejectMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x009d };
        RegisterRejectMsg(const PBYTEArray & pdu);
      protected:
        PUInt32l m_unknown;
    };

    class PortMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0002 };
        PortMsg(unsigned port);

      protected:
        PUInt16l m_port;
    };

    class CapabilityRequestMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x009B };
        CapabilityRequestMsg(const PBYTEArray & pdu);
    };

    class CapabilityResponseMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0010 };
        CapabilityResponseMsg(const OpalMediaFormatList & formats);

      protected:
        PUInt32l m_count;
        struct Info
        {
          PUInt32l m_codec;
          PUInt16l m_maxFramesPerPacket;
          char     m_reserved[10];
        } m_capability[32];
    };

    class KeepAliveMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0000 };
        KeepAliveMsg();
      protected:
        PUInt32l m_reserved;
    };

    class OffHookMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0006 };
        OffHookMsg(uint32_t line, uint32_t call);

      protected:
        PUInt32l m_lineInstance;
        PUInt32l m_callIdentifier;
    };

    class OnHookMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0007 };
        OnHookMsg(uint32_t line, uint32_t call);

      protected:
        PUInt32l m_lineInstance;
        PUInt32l m_callIdentifier;
    };

    class StartToneMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0082 };
        StartToneMsg(const PBYTEArray & pdu);

      protected:
        PUInt32l m_tone;
        PUInt32l m_reserved;
        PUInt32l m_lineInstance;
        PUInt32l m_callIdentifier;
    };

    class KeyPadButtonMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0003 };
        KeyPadButtonMsg(char button, uint32_t line, uint32_t call);

      protected:
        char     m_button;
        PUInt32l m_lineInstance;
        PUInt32l m_callIdentifier;
    };

    enum SkinnyCallStates
    {
      e_OnHookState    = 0x02,
      e_ConnectedState = 0x05,
      e_ProceedState   = 0x12
    };
    class CallStateMsg : public SkinnyMsg
    {
      public:
        enum { ID = 0x0111 };
        CallStateMsg(const PBYTEArray & pdu);

      SkinnyCallStates GetState() const { return (SkinnyCallStates)(uint32_t)m_state; }
      uint32_t GetLineIdentifier() const { return m_lineIdentifier; }
      uint32_t GetCallIdentifier() const { return m_callIdentifier; }

    protected:
      PUInt32l m_state;
      PUInt32l m_lineIdentifier;
      PUInt32l m_callIdentifier;
    };
#pragma pack()

    /** Send a message to server.
      */
    bool SendSkinnyMsg(const SkinnyMsg & msg);
  //@}

  protected:
    void HandleServerTransport();
    virtual void HandleRegisterAck(const RegisterAckMsg & msg);

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
      const PString & number,
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
  //@}

  /**@name Protocol handling routines */
  //@{
    typedef OpalSkinnyEndPoint::SkinnyMsg SkinnyMsg;

    /** Handle incoming message for this connection.
      */
    void HandleMessage(
      const SkinnyMsg & msg
    );
    //@}

  protected:
    OpalSkinnyEndPoint & m_endpoint;

    uint32_t m_lineInstance;
    uint32_t m_callidentifier;
    PString  m_numberToDial;

  friend class OpalSkinnyEndPoint;
};

#endif // OPAL_SKINNY

#endif // OPAL_SKINNY_H
