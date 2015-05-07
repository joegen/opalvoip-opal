/*
 * Python bindings.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2011 Demetrius Cassidy
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
 * The Original Code is Open Phone Abstraction Library (OPAL)
 *
 * The Initial Developer of the Original Code is Demetrius Cassidy
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

%ModuleHeaderCode
#include <ptlib.h>
#include <im/sipim.h>
%End


class OpalSIPIMMediaType :  OpalIMMediaType 
{
  public:
    OpalSIPIMMediaType();
    virtual OpalMediaSession * CreateMediaSession(OpalConnection & conn, unsigned sessionID) const;

    SDPMediaDescription * CreateSDPMediaDescription(const OpalTransportAddress & localAddress);
};

////////////////////////////////////////////////////////////////////////////

/** Class for carrying MSRP session information
  */
class OpalSIPIMMediaSession : OpalMediaSession
{

  public:
    OpalSIPIMMediaSession(OpalConnection & connection, unsigned sessionId);
    OpalSIPIMMediaSession(const OpalSIPIMMediaSession & _obj);

    virtual bool Open();

    virtual void Close();

    virtual PObject * Clone() const;

    virtual bool IsActive() const;

    virtual bool IsRTP() const;

    virtual bool HasFailed() const;

    virtual OpalTransportAddress GetLocalMediaAddress() const;

    virtual void SetRemoteMediaAddress(const OpalTransportAddress &, const OpalMediaFormatList & );

    virtual SDPMediaDescription * CreateSDPMediaDescription(
      const OpalTransportAddress & localAddress
    );

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, 
      unsigned sessionID, 
      PBoolean isSource
    );

    virtual PString GetCallID() const;
};

////////////////////////////////////////////////////////////////////////////

class OpalSIPIMContext : OpalConnectionIMContext
{
  public:
    OpalSIPIMContext();

    virtual SentStatus SendCompositionIndication(bool active = true);

    static void PopulateParams(SIPMessage::Params & params, OpalIM & message);

  protected:
    virtual SentStatus InternalSendOutsideCall(OpalIM * message);
    virtual SentStatus InternalSendInsideCall(OpalIM * message);

    virtual SentStatus OnIncomingIM(OpalIM & message);
    void OnCompositionIndicationTimeout();

    void ResetTimers(OpalIM & message);

    //PDECLARE_NOTIFIER(PTimer, OpalSIPIMContext, OnRxCompositionTimerExpire);
    //PDECLARE_NOTIFIER(PTimer, OpalSIPIMContext, OnTxCompositionTimerExpire);
    //PDECLARE_NOTIFIER(PTimer, OpalSIPIMContext, OnTxIdleTimerExpire);
};


////////////////////////////////////////////////////////////////////////////

