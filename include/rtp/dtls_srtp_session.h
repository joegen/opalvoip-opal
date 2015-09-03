/*
 * dtls_srtp_session.h
 *
 * SRTP protocol session handler with DTLS key exchange
 *
 * OPAL Library
 *
 * Copyright (C) 2014 Vox Lucida Pty. Ltd.
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
 * The Original Code is OPAL Library.
 *
 * The Initial Developer of the Original Code is Sysolyatin Pavel
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_RTP_DTLS_SRTP_SESSION_H
#define OPAL_RTP_DTLS_SRTP_SESSION_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#ifndef _PTLIB_H
#include <ptlib.h>
#endif

#include <opal_config.h>
#include <sdp/ice.h>
#include <rtp/srtp_session.h>

#if OPAL_SRTP
#include <ptclib/pssl.h>
#include <ptclib/pstun.h>
#include <ptclib/qchannel.h>


/**String option key to an integer indicating the time in seconds to
   wait for any DTLS messages. Default 2.
  */
#define OPAL_OPT_DTLS_TIMEOUT "DTLS-Timeout"


#if OPAL_ICE
typedef OpalICEMediaTransport OpalDTLSMediaTransportParent;
#else
typedef OpalUDPMediaStransport OpalDTLSMediaTransportParent;
#endif

class OpalDTLSMediaTransport : public OpalDTLSMediaTransportParent
{
    PCLASSINFO(OpalDTLSMediaTransport, OpalDTLSMediaTransportParent);
  public:
    OpalDTLSMediaTransport(const PString & name, bool passiveMode, const PSSLCertificateFingerprint& fp);
    ~OpalDTLSMediaTransport() { InternalStop(); }

    virtual bool Open(OpalMediaSession & session, PINDEX count, const PString & localInterface, const OpalTransportAddress & remoteAddress);
    virtual bool IsEstablished() const;
    virtual bool GetKeyInfo(OpalMediaCryptoKeyInfo * keyInfo[2]);

    void SetPassiveMode(bool passive) { m_passiveMode = passive; }
    PSSLCertificateFingerprint GetLocalFingerprint(PSSLCertificateFingerprint::HashType hashType) const;
    void SetRemoteFingerprint(const PSSLCertificateFingerprint& fp) { m_remoteFingerprint = fp; }

    class DTLSChannel : public PSSLChannelDTLS
    {
        PCLASSINFO(DTLSChannel, PSSLChannelDTLS);
      public:
        DTLSChannel(const OpalDTLSMediaTransport & transport);
#if PTRACING
        virtual int BioRead(char * buf, int len);
        virtual int BioWrite(const char * buf, int len);
#endif
    };

  protected:
    virtual void InternalOnStart(SubChannels subchannel);
    virtual DTLSChannel * CreateDTLSChannel();
    PDECLARE_SSLVerifyNotifier(OpalDTLSMediaTransport, OnVerify);

    bool            m_passiveMode;
    PTimeInterval   m_handshakeTimeout;
    unsigned        m_MTU;
    PSSLCertificate m_certificate;
    PSSLPrivateKey  m_privateKey;
    PSSLCertificateFingerprint m_remoteFingerprint;
    std::auto_ptr<OpalMediaCryptoKeyInfo> m_keyInfo[2];

  friend class OpalDTLSContext;
};


class OpalDTLSSRTPSession : public OpalSRTPSession
{
    PCLASSINFO(OpalDTLSSRTPSession, OpalSRTPSession);
  public:
    static const PCaselessString & RTP_DTLS_SAVP();
    static const PCaselessString & RTP_DTLS_SAVPF();

    OpalDTLSSRTPSession(const Init & init);
    ~OpalDTLSSRTPSession();

    virtual const PCaselessString & GetSessionType() const { return RTP_DTLS_SAVP(); }

    // New members
    void SetPassiveMode(bool passive);
    bool IsPassiveMode() const { return m_passiveMode; }

    PSSLCertificateFingerprint GetLocalFingerprint(PSSLCertificateFingerprint::HashType hashType) const;
    void SetRemoteFingerprint(const PSSLCertificateFingerprint& fp);

  protected:
    virtual OpalMediaTransport * CreateMediaTransport(const PString & name);

    bool                       m_passiveMode;
    PSSLCertificateFingerprint m_remoteFingerprint;
};


#endif // OPAL_SRTP

#endif // OPAL_RTP_DTLS_SRTP_SESSION_H
