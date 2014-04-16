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


class OpalDTLSSRTPSession : public OpalSRTPSession
{
    PCLASSINFO(OpalDTLSSRTPSession, OpalSRTPSession);
  public:
    static const PCaselessString & RTP_DTLS_SAVP();
    static const PCaselessString & RTP_DTLS_SAVPF();

    OpalDTLSSRTPSession(const Init & init);
    ~OpalDTLSSRTPSession();
    virtual const PCaselessString & GetSessionType() const { return RTP_DTLS_SAVP(); }
    virtual void SetRemoteUserPass(const PString & user, const PString & pass);
    void SetConnectionInitiator(bool connectionInitiator);
    bool GetConnectionInitiator() const;
    const PSSLCertificateFingerprint& GetLocalFingerprint() const;
    void SetRemoteFingerprint(const PSSLCertificateFingerprint& fp);
    const PSSLCertificateFingerprint& GetRemoteFingerprint() const;
    void SetCandidates(const PNatCandidateList& candidates);
  protected:
    virtual SendReceiveStatus ReadRawPDU(BYTE * framePtr, PINDEX & frameSize, bool fromDataChannel);
    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame, bool rewriteHeader);
    virtual SendReceiveStatus OnSendControl(RTP_ControlFrame & frame);
    virtual SendReceiveStatus OnReceiveControl(RTP_ControlFrame & frame);
    PDECLARE_NOTIFIER(PSSLChannelDTLS, OpalDTLSSRTPSession, OnHandshake);
    PDECLARE_SSLVerifyNotifier(OpalDTLSSRTPSession, OnVerify);
private:
    SendReceiveStatus IceNegotiation(const BYTE * framePtr, PINDEX frameSize, bool forDataChannel, bool isReceive);
    SendReceiveStatus HandshakeIfNeeded(const BYTE * framePtr, PINDEX frameSize, bool forDataChannel, bool isReceive);
    SendReceiveStatus ProcessPacket(const BYTE * framePtr, PINDEX frameSize, bool forDataChannel, bool isReceive);
    // Return true if send BindingResponse, else - false
    bool SendStunResponse(const PSTUNMessage& request, PUDPSocket& socket);
    bool SendStunRequest(PUDPSocket& socket, const PIPSocketAddressAndPort& ap);
    bool CheckStunResponse(const PSTUNMessage& response);
  private:
    struct Candidate {
      PIPSocketAddressAndPort ap;
      unsigned otherPartyResponses; // Count of responses to our STUN requests.
      unsigned otherPartyRequests; // Count of incoming STUN requests
      unsigned ourResponses; // Count of incoming STUN responses
      Candidate(const PIPSocketAddressAndPort& addressAndPort)
        : ap(addressAndPort)
        , otherPartyResponses(0)
        , otherPartyRequests(0)
        , ourResponses(0)
      {
      }
      bool Ready() const
      {
        return (otherPartyRequests == ourResponses
          && ourResponses > 1
          && otherPartyResponses > 1);
      }
    };
    typedef std::list<Candidate> Candidates;

  private:
    std::auto_ptr<PSSLChannelDTLS> m_channels[2]; // Media and control channels
    Candidates m_candidates[2]; // Candidates for media and control channels
    bool m_dtlsReady[2]; // Ready flag for media and control channels
    bool m_stopSend[2];
    bool m_connectionInitiator;
    PSSLCertificateFingerprint m_remoteFingerprint;
    bool m_stunNegotiated;
    PSTUN m_remoteStun; // Used for receive requests from other party and send responses
    PSTUN m_localStun; // Used for send request to other party and handle responses
};


#endif // OPAL_SRTP

#endif // OPAL_RTP_DTLS_SRTP_SESSION_H
