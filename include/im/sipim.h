/*
 * sipim.h
 *
 * Support for SIP session mode IM
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2008 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_IM_SIPIM_H
#define OPAL_IM_SIPIM_H

#include <ptlib.h>
#include <opal/buildopts.h>
#include <opal/rtpconn.h>
#include <opal/manager.h>
#include <opal/mediastrm.h>
#include <opal/mediatype.h>
#include <opal/mediatype.h>
#include <im/im.h>
#include <sip/sdp.h>
#include <sip/sippdu.h>

#if OPAL_HAS_SIPIM

class OpalSIPIMMediaType : public OpalIMMediaType 
{
  public:
    OpalSIPIMMediaType();

    virtual OpalMediaSession * CreateMediaSession(OpalConnection & conn, unsigned sessionID) const;

    SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress,
      OpalMediaSession * session
    ) const;
};

////////////////////////////////////////////////////////////////////////////

/** Class for carrying MSRP session information
  */
class OpalSIPIMMediaSession : public OpalMediaSession
{
  PCLASSINFO(OpalSIPIMMediaSession, OpalMediaSession);
  public:
    OpalSIPIMMediaSession(OpalConnection & connection, unsigned sessionId);

    virtual PObject * Clone() const { return new OpalSIPIMMediaSession(*this); }

    virtual OpalTransportAddress GetLocalMediaAddress() const;
    virtual OpalTransportAddress GetRemoteMediaAddress() const;

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      PBoolean isSource
    );

    virtual PString GetCallID() const { return m_callId; }

  protected:
    OpalTransportAddress m_transportAddress;
    SIPURL               m_localURL;
    SIPURL               m_remoteURL;
    PString              m_callId;

  private:
    OpalSIPIMMediaSession(const OpalSIPIMMediaSession & obj) : OpalMediaSession(obj.m_connection, obj.m_sessionId, obj.m_mediaType) { }
    void operator=(const OpalSIPIMMediaSession &) { }

  friend class OpalSIPIMMediaType;
};

////////////////////////////////////////////////////////////////////////////

class OpalSIPIMContext : public OpalIMContext
{
  public:
    OpalSIPIMContext();

    virtual bool SendCompositionIndication(const CompositionInfo & info);

    static void OnMESSAGECompleted(
      SIPEndPoint & endpoint,
      const SIPMessage::Params & params,
      SIP_PDU::StatusCodes reason
    );
    static void OnReceivedMESSAGE(
      SIPEndPoint & endpoint,
      SIPConnection * connection,
      OpalTransport & transport,
      SIP_PDU & pdu
    );

  protected:
    virtual MessageDisposition InternalSendOutsideCall(OpalIM & message);
    virtual MessageDisposition InternalSendInsideCall(OpalIM & message);

    virtual MessageDisposition OnMessageReceived(const OpalIM & message);

    virtual MessageDisposition InternalOnCompositionIndication(const OpalIM & message);
    virtual MessageDisposition InternalOnDisposition(const OpalIM & message);

    void PopulateParams(SIPMessage::Params & params, const OpalIM & message);

    PDECLARE_NOTIFIER(PTimer, OpalSIPIMContext, OnRxCompositionIdleTimer);
    PDECLARE_NOTIFIER(PTimer, OpalSIPIMContext, OnTxCompositionIdleTimer);

    PString m_rxCompositionState;
    PTimer m_rxCompositionIdleTimeout;
    PString m_txCompositionState;
    PTimer m_txCompositionIdleTimeout;
    PSimpleTimer m_txCompositionRefreshTimeout;
    PTime m_lastActive;

    RFC4103Context m_rfc4103Context;
};


////////////////////////////////////////////////////////////////////////////

#endif // OPAL_HAS_SIPIM

#endif // OPAL_IM_SIPIM_H
