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


class OpalRTPSession;
class H46019_TraversalParameters;


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
    virtual bool OnSendFacility_UUIE      (H460_FeatureDescriptor & /*pdu*/) { return !m_disabledByH46024; }

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
    enum { DefaultPriority = 20 };
    PNatMethod_H46019(unsigned priority = DefaultPriority);

    /**@name Overrides from PNatMethod */
    //@{
    /**  GetName
        Get the NAT method name 
    */
    virtual PCaselessString GetMethodName() const;
    static const char * MethodName();

    /**Get the current server address name.
      */
    virtual PString GetServer() const;

    /**  isAvailable.
        Returns whether the Nat Method is ready and available in
        assisting in NAT Traversal. The principal is function is
        to allow the EP to detect various methods and if a method
        is detected then this method is available for NAT traversal
        The Order of adding to the PNstStrategy determines which method
        is used
    */
    virtual bool IsAvailable(const PIPSocket::Address & address, PObject * userData);
    //@}

  protected:
    virtual PNATUDPSocket * InternalCreateSocket(Component component, PObject * context);
    virtual void InternalUpdate();
};


///////////////////////////////////////////////////////////////////////////////

class H46019UDPSocket : public PNATUDPSocket
{
    PCLASSINFO(H46019UDPSocket, PNATUDPSocket);
  public:
    /**@name Construction/Deconstructor */
    //@{
    /** create a UDP Socket Fully Nat Supported
        ready for H323plus to Call.
    */
    H46019UDPSocket(
      PNatMethod::Component component,
      OpalRTPSession & session
    );

    ~H46019UDPSocket();
    //@}

    /**@name Functions */
    //@{
    /** Activate (FastStart) keep-alive mechanism.
    */
    void Activate();

    /** Activate keep-alive mechanism.
    */
    void Activate(
      const H46019_TraversalParameters & params
    );

    /** Get the Ping Payload
      */
    RTP_DataFrame::PayloadTypes GetPingPayload() const { return m_keepAlivePayloadType; }

    /** Set the Ping Payload
      */
    void SetPingPayLoad(RTP_DataFrame::PayloadTypes val) { m_keepAlivePayloadType = val; }

    /** Get Ping TTL
      */
    unsigned GetTTL() const { return m_keepAliveTTL.GetSeconds(); }

    /** Set Ping TTL
      */
    void SetTTL(unsigned val) { m_keepAliveTTL.SetInterval(0, val); }

  protected:
    bool InternalWriteTo(const Slice * slices, size_t sliceCount, const PIPSocketAddressAndPort & ipAndPort);

#if H46019_SERVER_SUPPORT
    bool InternalReadFrom(Slice * slices, size_t sliceCount, PIPSocketAddressAndPort & ipAndPort);
#endif

  // Memeber variables
    H323Connection & m_connection;

    unsigned m_sessionID;             ///< Current Session ie 1-Audio 2-video

    // H.460.19 keep alives
    PIPSocketAddressAndPort     m_keepAliveAddress;     ///< KeepAlive Address
    RTP_DataFrame::PayloadTypes m_keepAlivePayloadType; ///< KeepAlive RTP payload
    PTimeInterval               m_keepAliveTTL;         ///< KeepAlive TTL
    WORD                        m_keepAliveSequence;    ///< KeepAlive sequence number

    PDECLARE_NOTIFIER(PTimer, H46019UDPSocket, Ping);   ///< Timer to notify to poll for External IP
    PTimer m_keepAliveTimer;                            ///< Polling Timer
    void SendPing(const PIPSocketAddressAndPort & ipAndPort);
    bool SendPacket(const PBYTEArray & data, const PIPSocketAddressAndPort & ipAndPort);

    // H.460.19 multiplex transmit
    bool     m_multiplexedTransmit;
    PUInt32b m_multiplexID;

#if H46024_ANNEX_A_SUPPORT
    // H46024 Annex A support
    struct probe_packet {
        PUInt16b    Length;   // Length
        PUInt32b    SSRC;     // Time Stamp
        BYTE        name[4];  // Name is limited to 32 (4 Bytes)
        BYTE        cui[20];  // SHA-1 is always 160 (20 Bytes)
    };

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

    /** Set Alternate Direct Address
      */
    virtual void SetAlternateAddresses(const H323TransportAddress & address, const PString & cui);

     /** Set Alternate Direct Address
      */
    virtual void GetAlternateAddresses(H323TransportAddress & address, PString & cui);

    /** Start sending media/control to alternate address
      */
    void H46024Adirect(bool starter);

    /** Start Probing to alternate address
      */
    void H46024Bdirect(const H323TransportAddress & address);
    PBoolean ReceivedProbePacket(const RTP_ControlFrame & frame, bool & probe, bool & success);
    void BuildProbe(RTP_ControlFrame & report, bool reply);
    void StartProbe();
    void ProbeReceived(bool probe, const PIPSocketAddressAndPort & ipAndPort);

    // H46024 Annex A support
    PString  m_localCUI;                                ///< Local CUI (for H.460.24 Annex A)
    PString m_remoteCUI;                                ///< Remote CUI
    probe_state m_state;
    PMutex probeMutex;
    PIPSocketAddressAndPort m_detectedAddress;          ///< detected remote Address (as detected from actual packets)
    PIPSocketAddressAndPort m_pendingAddress;           ///< detected pending RTCP Probe Address (as detected from actual packets)
    PDECLARE_NOTIFIER(PTimer, H46019UDPSocket, Probe);  ///< Thread to probe for direct connection
    PTimer m_Probe;                                     ///< Probe Timer
    PINDEX m_probes;                                    ///< Probe count
    DWORD m_SSRC;                                         ///< Random number
    PIPSocket::Address m_altAddr;  WORD m_altPort;      ///< supplied remote Address (as supplied in Generic Information)

    // H46024 Annex B support
    bool m_h46024b;
#endif
};


#endif // OPAL_H460_NAT

#endif // OPAL_H460_STD19_H
