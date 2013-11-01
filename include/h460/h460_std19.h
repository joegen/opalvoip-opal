/*
 * h460_std19.h
 *
 * Copyright (c) 2009 ISVO (Asia) Pte Ltd. All Rights Reserved.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the General Public License (the  "GNU License"), in which case the
 * provisions of GNU License are applicable instead of those
 * above. If you wish to allow use of your version of this file only
 * under the terms of the GNU License and not to allow others to use
 * your version of this file under the MPL, indicate your decision by
 * deleting the provisions above and replace them with the notice and
 * other provisions required by the GNU License. If you do not delete
 * the provisions above, a recipient may use your version of this file
 * under either the MPL or the GNU License."
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is derived from and used in conjunction with the 
 * H323Plus Project (www.h323plus.org/)
 *
 * The Initial Developer of the Original Code is ISVO (Asia) Pte Ltd.
 *
 * Contributor(s): Many thanks to Simon Horne.
 *                 Robert Jongbloed (robertj@voxlucida.com.au).
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_H460_STD19_H
#define OPAL_H460_STD19_H

#include <h460/h4601.h>

#if OPAL_H460_NAT

#include <rtp/rtp.h>


#if _MSC_VER
#pragma once
#endif

class H460_FeatureStd19 : public H460_Feature
{
    PCLASSINFO(H460_FeatureStd19, H460_Feature);
  public:
    H460_FeatureStd19();

    // Universal Declarations Every H460 Feature should have the following
    static Purpose GetPluginPurpose()      { return ForConnection; };
    virtual Purpose GetPurpose() const     { return GetPluginPurpose(); };

    virtual bool Initialise(H323EndPoint & ep, H323Connection * con);

    // H.225.0 Messages
    virtual bool OnSendSetup_UUIE         (H460_FeatureDescriptor & /*pdu*/) { return !m_disabledByH46024; }
    virtual bool OnSendCallProceeding_UUIE(H460_FeatureDescriptor & /*pdu*/) { return !m_disabledByH46024; }
    virtual bool OnSendAlerting_UUIE      (H460_FeatureDescriptor & /*pdu*/) { return !m_disabledByH46024; }
    virtual bool OnSendCallConnect_UUIE   (H460_FeatureDescriptor & /*pdu*/) { return !m_disabledByH46024; }

    virtual bool OnSendingOLCGenericInformation(unsigned sessionID, H245_ArrayOf_GenericParameter & param, bool isAck);
    virtual void OnReceiveOLCGenericInformation(unsigned sessionID, const H245_ArrayOf_GenericParameter & param);

    // H.460.24 Override
    void DisableByH46024() { m_disabledByH46024 = true; }

  protected:
    PNatMethod * m_natMethod;
    bool         m_disabledByH46024;
};
 

///////////////////////////////////////////////////////////////////////////////

class PNatMethod_H46019 : public PNatMethod
{
    PCLASSINFO(PNatMethod_H46019, PNatMethod);
  public:
    enum { DefaultPriority = 50 };
    PNatMethod_H46019(unsigned priority = DefaultPriority);

    /**@name Overrides from PNatMethod */
    //@{
    /**  GetName
        Get the NAT method name 
    */
    virtual PCaselessString GetName() const;
    static const char * MethodName();

    /**Get the current server address name.
      */
    virtual PString GetServer() const;

    /**  CreateSocketPair
        Create the UDP Socket pair
    */
    virtual PBoolean CreateSocketPair(
      PUDPSocket * & socket1,            ///< Created DataSocket
      PUDPSocket * & socket2,            ///< Created ControlSocket
      const PIPSocket::Address & binding,  
      void * userData
    );

    /**  isAvailable.
        Returns whether the Nat Method is ready and available in
        assisting in NAT Traversal. The principal is function is
        to allow the EP to detect various methods and if a method
        is detected then this method is available for NAT traversal
        The Order of adding to the PNstStrategy determines which method
        is used
    */
    virtual bool IsAvailable(const PIPSocket::Address & address);

    /**  OpenSocket
        Create a single UDP Socket 
    */
    PBoolean OpenSocket(PUDPSocket & socket, PortInfo & portInfo, const PIPSocket::Address & binding) const;
    //@}

  protected:
    NatTypes InternalGetNatType(bool, const PTimeInterval &);
};


///////////////////////////////////////////////////////////////////////////////

class H46019UDPSocket : public PUDPSocket
{
    PCLASSINFO(H46019UDPSocket, PUDPSocket);
  public:
    /**@name Construction/Deconstructor */
    //@{
    /** create a UDP Socket Fully Nat Supported
        ready for H323plus to Call.
    */
    H46019UDPSocket(
      H323Connection & connection,
      unsigned sessionID,
      bool rtp
    );

    /** Deconstructor to reallocate Socket and remove any exiting
        allocated NAT ports, 
    */
    ~H46019UDPSocket();
    //@}

    PBoolean GetLocalAddress(Address & addr, WORD & port);

    /**@name Functions */
    //@{

    /** Allocate (FastStart) keep-alive mechanism but don't Activate
    */
    void Allocate(
      const H323TransportAddress & keepalive, 
      unsigned _payload, 
      unsigned _ttl,
      unsigned _muxId
    );

    /** Activate (FastStart) keep-alive mechanism.
    */
    void Activate();

    /** Activate keep-alive mechanism.
    */
    void Activate(const H323TransportAddress & keepalive,    ///< KeepAlive Address
      unsigned _payload,            ///< RTP Payload type    
      unsigned _ttl,                ///< Time interval for keepalive.
      unsigned _muxId
    );

    /** Get the Ping Payload
      */
    unsigned GetPingPayload();

    /** Set the Ping Payload
      */
    void SetPingPayLoad(unsigned val);

    /** Get Ping TTL
      */
    unsigned GetTTL();

    /** Set Ping TTL
      */
    void SetTTL(unsigned val);

     /**Read a datagram from a remote computer
       @return true if any bytes were sucessfully read.
       */
    virtual PBoolean ReadFrom(
      void * buf,     ///< Data to be written as URGENT TCP data.
      PINDEX len,     ///< Number of bytes pointed to by #buf#.
      Address & addr, ///< Address from which the datagram was received.
      WORD & port     ///< Port from which the datagram was received.
    );

    /**Write a datagram to a remote computer.
       @return true if all the bytes were sucessfully written.
     */
    virtual PBoolean WriteTo(
      const void * buf,     ///< Data to be written as URGENT TCP data.
      PINDEX len,           ///< Number of bytes pointed to by #buf#.
      const Address & addr, ///< Address to which the datagram is sent.
      WORD port             ///< Port to which the datagram is sent.
    );

    enum  probe_state {
        e_notRequired,        ///< Polling has not started
        e_initialising,       ///< We are initialising (local set but remote not)
        e_idle,               ///< Idle (waiting for first packet from remote)
        e_probing,            ///< Probing for direct route
        e_verify_receiver,    ///< verified receive connectivity    
        e_verify_sender,      ///< verified send connectivity
        e_wait,               ///< we are waiting for direct media (to set address)
        e_direct              ///< we are going direct to detected address
    };

    struct probe_packet {
        PUInt16b    Length;   // Length
        PUInt32b    SSRC;     // Time Stamp
        BYTE        name[4];  // Name is limited to 32 (4 Bytes)
        BYTE        cui[20];  // SHA-1 is always 160 (20 Bytes)
    };

    /** Set Alternate Direct Address
      */
    virtual void SetAlternateAddresses(const H323TransportAddress & address, const PString & cui);

     /** Set Alternate Direct Address
      */
    virtual void GetAlternateAddresses(H323TransportAddress & address, PString & cui);

    /** Callback to check if the address received is a permitted alternate
      */
    virtual PBoolean IsAlternateAddress(
        const Address & address,    ///< Detected IP Address.
        WORD port                    ///< Detected Port.
        );
    /** Start sending media/control to alternate address
      */
    void H46024Adirect(bool starter);

    /** Start Probing to alternate address
      */
    void H46024Bdirect(const H323TransportAddress & address);
    //@}

  protected:
    // H.460.19 Keepalives
    void InitialiseKeepAlive();    ///< Start the keepalive
    void SendRTPPing(const PIPSocket::Address & ip, const WORD & port);
    void SendRTCPPing();
    PBoolean SendRTCPFrame(RTP_ControlFrame & report, const PIPSocket::Address & ip, WORD port);
    PMutex PingMutex;

    // H46024 Annex A support
    PBoolean ReceivedProbePacket(const RTP_ControlFrame & frame, bool & probe, bool & success);
    void BuildProbe(RTP_ControlFrame & report, bool reply);
    void StartProbe();
    void ProbeReceived(bool probe, const PIPSocket::Address & addr, WORD & port);
    void SetProbeState(probe_state newstate);
    int GetProbeState() const;
    probe_state m_state;
    PMutex probeMutex;

    /**Write a datagram to a remote computer.
       @return true if all the bytes were sucessfully written.
     */
    virtual PBoolean Internal_WriteTo(
      const Slice * slices, ///< Data to be written as array of slices.
      size_t nSlices,       ///< Number of #slices#.
      const Address & addr, ///< Address to which the datagram is sent.
      WORD port             ///< Port to which the datagram is sent.
    );
  
    /**Write a datagram to a remote computer.
   @return true if all the bytes were sucessfully written.
     */
    virtual PBoolean Internal_WriteTo(
      const void * buf,     ///< Data to be written.
      PINDEX len,           ///< Number of bytes pointed to by #buf#.
      const Address & addr, ///< Address to which the datagram is sent.
      WORD port             ///< Port to which the datagram is sent.
    );

  private:
    H323Connection & m_connection;
    unsigned m_sessionID;             ///< Current Session ie 1-Audio 2-video
    PString m_CUI;                  ///< Local CUI (for H.460.24 Annex A)

    // H.460.19 Keepalives
    PIPSocket::Address keepip;      ///< KeepAlive Address
    WORD keepport;                  ///< KeepAlive Port
    unsigned keeppayload;           ///< KeepAlive RTP payload
    unsigned keepTTL;               ///< KeepAlive TTL
    PUInt32b muxId;                 ///< MuxId 
    WORD keepseqno;                 ///< KeepAlive sequence number
    PTime * keepStartTime;          ///< KeepAlive start time for TimeStamp.

    PDECLARE_NOTIFIER(PTimer, H46019UDPSocket, Ping);   ///< Timer to notify to poll for External IP
    PTimer    Keep;                                     ///< Polling Timer

    // H46024 Annex A support
    PString m_CUIrem;                                   ///< Remote CUI
    PIPSocket::Address m_locAddr;  WORD m_locPort;      ///< local Address (address used when starting socket)
    PIPSocket::Address m_remAddr;  WORD m_remPort;      ///< Remote Address (address used when starting socket)
    PIPSocket::Address m_detAddr;  WORD m_detPort;      ///< detected remote Address (as detected from actual packets)
    PIPSocket::Address m_pendAddr;  WORD m_pendPort;    ///< detected pending RTCP Probe Address (as detected from actual packets)
    PDECLARE_NOTIFIER(PTimer, H46019UDPSocket, Probe);  ///< Thread to probe for direct connection
    PTimer m_Probe;                                     ///< Probe Timer
    PINDEX m_probes;                                    ///< Probe count
    DWORD m_SSRC;                                         ///< Random number
    PIPSocket::Address m_altAddr;  WORD m_altPort;      ///< supplied remote Address (as supplied in Generic Information)
    // H46024 Annex B support
    PBoolean    m_h46024b;

    bool m_rtpSocket;
};


#endif // OPAL_H460_NAT

#endif // OPAL_H460_STD19_H
