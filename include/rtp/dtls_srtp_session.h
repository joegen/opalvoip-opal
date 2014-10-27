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
#include <rtp/srtp_session.h>

#if OPAL_SRTP
#include <ptclib/pssl.h>
#include <ptclib/pstun.h>
#include <ptclib/qchannel.h>


class OpalDTLSSRTPSession : public OpalSRTPSession
{
    PCLASSINFO(OpalDTLSSRTPSession, OpalSRTPSession);
  public:
    static const PCaselessString & RTP_DTLS_SAVP();
    static const PCaselessString & RTP_DTLS_SAVPF();

    OpalDTLSSRTPSession(const Init & init);
    ~OpalDTLSSRTPSession();

    virtual const PCaselessString & GetSessionType() const { return RTP_DTLS_SAVP(); }

    // OVerrides from OpalRTPSession
    virtual bool Open(const PString & localInterface, const OpalTransportAddress & remoteAddress, bool mediaAddress);
    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame, RewriteMode rewrite);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame);

    // New members
    void SetPassiveMode(bool passive);
    bool IsPassiveMode() const { return m_passiveMode; }

    PSSLCertificateFingerprint GetLocalFingerprint() const;
    void SetRemoteFingerprint(const PSSLCertificateFingerprint& fp);
    const PSSLCertificateFingerprint& GetRemoteFingerprint() const;

  protected:
    class SSLChannel;

    virtual void ThreadMain();
    virtual void InternalClose();
    virtual bool ExecuteHandshake(Channel channel);
    PDECLARE_SSLVerifyNotifier(OpalDTLSSRTPSession, OnVerify);


    bool                       m_passiveMode;
    PSSLCertificateFingerprint m_remoteFingerprint;

    // One each for Media and Control channels
    SSLChannel  * m_sslChannel[2];
};


#endif // OPAL_SRTP

#endif // OPAL_RTP_DTLS_SRTP_SESSION_H
