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
    PCLASSINFO_WITH_CLONE(H460_FeatureStd19, H460_Feature);
  public:
    H460_FeatureStd19();

    static const H460_FeatureID & ID();
    virtual bool Initialise(H323EndPoint & ep, H323Connection * con);

    // H.225.0 Messages
    virtual bool OnSendPDU(H460_MessageType pduType, H460_FeatureDescriptor & pdu);
    virtual void OnReceivePDU(H460_MessageType pduType, const H460_FeatureDescriptor & pdu);

    virtual bool OnSendingOLCGenericInformation(unsigned sessionID, H245_ArrayOf_GenericParameter & param, bool isAck);
    virtual void OnReceiveOLCGenericInformation(unsigned sessionID, const H245_ArrayOf_GenericParameter & param, bool isAck);

    // Member access
    bool IsRemoteServer() const { return m_remoteIsServer; }

  protected:
    PNatMethod * m_natMethod;
    bool         m_disabledByH46024;
    bool         m_remoteSupportsMux;
    bool         m_remoteIsServer;
};
 

///////////////////////////////////////////////////////////////////////////////

class H46019Server : public PObject
{
    PCLASSINFO(H46019Server, PObject)
  public:
    H46019Server(H323EndPoint & ep);
    H46019Server(
      H323EndPoint & ep,
      const PIPSocket::Address & localInterface
    );
    ~H46019Server();

    unsigned GetKeepAliveTTL() const { return m_keepAliveTTL; }
    void SetKeepAliveTTL(unsigned val) { m_keepAliveTTL = val; }

    H323TransportAddress GetKeepAliveAddress() const;
    H323TransportAddress GetMuxRTPAddress() const;
    H323TransportAddress GetMuxRTCPAddress() const;

    unsigned CreateMultiplexID(
      const PIPSocketAddressAndPort & rtp,
      const PIPSocketAddressAndPort & rtcp
    );

  protected:
    H323EndPoint & m_endpoint;
    unsigned       m_keepAliveTTL;         ///< KeepAlive TTL

    PUDPSocket * m_keepAliveSocket;
    PUDPSocket * m_rtpSocket;
    PUDPSocket * m_rtcpSocket;

    struct SocketPair {
      SocketPair(const PIPSocketAddressAndPort & rtp, const PIPSocketAddressAndPort & rtcp) : m_rtp(rtp), m_rtcp(rtcp) { }
      PIPSocketAddressAndPort m_rtp;
      PIPSocketAddressAndPort m_rtcp;
    };
    typedef map<uint32_t, SocketPair> MuxMap;
    MuxMap m_multiplexedSockets;

    PThread * m_multiplexThread;
    void MultiplexThread();
    bool Multiplex(bool rtcp);
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
    virtual bool InternalGetServerAddress(PIPSocketAddressAndPort & ap) const;
    virtual PNATUDPSocket * InternalCreateSocket(Component component, PObject * context);
    virtual void InternalUpdate(bool);
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
    virtual const char * GetNatName() const;

    /** Activate keep-alive mechanism.
    */
    void ActivateKeepAliveRTP(
      const H323TransportAddress & address,
      unsigned ttl
    );

    void ActivateKeepAliveRTCP(unsigned ttl);

    void SetMultiplexID(unsigned multiplexID) { m_multiplexedTransmit = true; m_multiplexID = multiplexID; }

    RTP_DataFrame::PayloadTypes FindKeepAlivePayloadType(H323Connection & connection);

  protected:
    bool InternalWriteTo(const Slice * slices, size_t sliceCount, const PIPSocketAddressAndPort & ipAndPort);

  // Memeber variables
    OpalRTPSession & m_session;

    // H.460.19 keep alives
    PIPSocketAddressAndPort     m_keepAliveAddress;     ///< KeepAlive Address
    RTP_DataFrame::PayloadTypes m_keepAlivePayloadType; ///< KeepAlive RTP payload
    PTimeInterval               m_keepAliveTTL;         ///< KeepAlive TTL
    WORD                        m_keepAliveSequence;    ///< KeepAlive sequence number

    PDECLARE_NOTIFIER(PTimer, H46019UDPSocket, KeepAliveTimeout);
    PTimer m_keepAliveTimer;
    void SendKeepAliveRTP(const PIPSocketAddressAndPort & ipAndPort);

    // H.460.19 multiplex transmit
    bool     m_multiplexedTransmit;
    PUInt32b m_multiplexID;
};


#endif // OPAL_H460_NAT

#endif // OPAL_H460_STD19_H
