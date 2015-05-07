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
#include <opal_config.h>

#if OPAL_HAS_SIPIM

#include <opal/mediasession.h>
#include <sip/sippdu.h>
#include <im/im.h>
#include <im/rfc4103.h>


////////////////////////////////////////////////////////////////////////////

/** Class representing a SIP Instant Messaging "conversation".
    This keeps the context for Instant Messages between two SIP entities. As
    there are, at least, three (and a half) different mechanisms for doing
    IM between two SIP endpoints these are chose via capabilities selected
    before calling OpalIMContext::Create()

    SIP-IM  Indicates RFC 3428 compliant messaging, using the SIP MESSAGE
            command. This may be done in call or out of call, the latter is
            the default. The in call mode is used if OpalIMEndPoint::Create()
            is given an existing SIPConnection in which to exchange messages.
    T.140   Indicates RFC 4103 compliant messaging, which is T.140 compliant
            text sent via RTP. This always requires an active connection, so
            one will be created if needed.
    MSRP    Indicates RFC 4975 compliant messaging.

    The selection on the method that is used is dependent on the applications
    active media formats.
  */
class OpalSIPIMContext : public OpalIMContext
{
  public:
    OpalSIPIMContext();

    virtual bool Open(bool byRemote);
    virtual bool SendCompositionIndication(const CompositionInfo & info);

    static void OnMESSAGECompleted(
      SIPEndPoint & endpoint,
      const SIPMessage::Params & params,
      SIP_PDU::StatusCodes reason
    );
    static void OnReceivedMESSAGE(
      SIPEndPoint & endpoint,
      SIPConnection * connection,
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

    PString      m_rxCompositionState;
    PTimer       m_rxCompositionIdleTimeout;
    PString      m_txCompositionState;
    PTimer       m_txCompositionIdleTimeout;
    PSimpleTimer m_txCompositionRefreshTimeout;
    PTime        m_lastActive;
};


////////////////////////////////////////////////////////////////////////////

#endif // OPAL_HAS_SIPIM

#endif // OPAL_IM_SIPIM_H
